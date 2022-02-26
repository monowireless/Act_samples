/* Copyright (C) 2022      Mono Wireless Inc. All Rights Reserved.  *
 * Released under MW-SLA-*J,*E (MONO WIRELESS SOFTWARE LICENSE      *
 * AGREEMENT).                                                      */


#ifndef SOURCE_DUP_CHECKER_HPP_
#define SOURCE_DUP_CHECKER_HPP_

#include <TWELITE>

#include <EASTL/utility.h>
#include <EASTL/intrusive_hash_map.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/bonus/ring_buffer.h>

#include "common.h"

#if DEBUG_DUP_CHECKER == 1
extern void test_dup_checker(); // just for checking behavior of this class
#endif

/**
 * MODE: 0x10000 -> reject packet with same value.
 *       0x2000X -> accept only newer value (X is bit length of value, e.g. X=6 value is 0..63)
 * TIMEOUT_ms: timeout value (note: value are truncated by dividing 64)
 * N_ENTRIES:
 */
template <unsigned MODE=0x10000, unsigned TIMEOUT_ms=1000, size_t N_ENTRIES=63, size_t N_BUCKET_HASH = 13>
class dup_check {
public:
	static const unsigned MODE_REJECT_SAME_SEQ = 0x10000;
	static const unsigned MODE_REJECT_OLDER_SEQ = 0x20000;

	// node key (32bit serial number)
	typedef uint32_t tkey;

	// node entry
	struct _dup_checker_entry : public eastl::intrusive_hash_node_key<tkey> {
		uint8_t _u8_vect_idx;    // index to the vector pool of this object.
		uint8_t _u8_ts;          // received timestamp
		uint16_t _u16_value;      // sequence number of this object.
	};

	using tvect = eastl::fixed_vector<_dup_checker_entry, N_ENTRIES, false>;
	using tmap = eastl::intrusive_hash_multimap<tkey, _dup_checker_entry, N_BUCKET_HASH>;
	using tring = eastl::ring_buffer< uint8_t, eastl::fixed_vector<uint8_t, N_ENTRIES+1, false> >;

private:
	tvect _vect_pool;       // vector pool that stores entries (_dup_checker_entry)
	tring _ring_vecant_idx; // manages vacant indexes in _vect_pool. (pop an entry when used, push an entry when released)
	tmap _mmap_entries;     // multi map entries (to find node by 32bit serial number)

	// convert serial value from 32bit ms value into 8bit (64ms/tick).
	uint8_t TS_TOu8(uint32_t u32ts) { return (u32ts >> 6) & 0xFF; }
	// time difference between 32bit ms value and 8bit tick.
	uint32_t TS_DIFF(uint32_t now, uint8_t past) { return ((TS_TOu8(now) - past) & 0xFFu) << 6; }


	/**
	 * @brief add a new node
	 *   when the node is accepted (not determined as duplicated),
	 *   request vacant node index from ring buffer and put this entry into mmap.
	 * 
	 * @param u32ser         32bit Serial number
	 * @param u16value       item value (node seq number or etc.)
	 * @param u32_timestamp  ms time count when received
	 * @return true          successfully added
	 * @return false         failed
	 */
	bool _insert_entry(uint32_t u32ser, uint16_t u16value, uint32_t u32_timestamp) {
		if (_ring_vecant_idx.empty()) return false; // no more pool

		unsigned idx = _ring_vecant_idx.front();
		_ring_vecant_idx.pop_front();

		// use an object in vector pool.
		auto& x = _vect_pool[idx];

		// key data and entry data
		x.mKey = u32ser;
		x._u16_value = u16value;
		x._u8_ts = TS_TOu8(u32_timestamp);

		_mmap_entries.insert(x);

		return true;
	}

	/**
	 * @brief remove a node.
	 *   remove the node (x) and take the index into ring buffer.
	 *
	 * @param x         the node body
	 */
	void _delete_entry(_dup_checker_entry &x) {
		_mmap_entries.remove(x);                    // remove from mmap
		_ring_vecant_idx.push_back(x._u8_vect_idx); // return the index to ringbuffer
	}

public:
	/**
	 * @brief the constructor.
	 *   - prepare all node bodies into vector.
	 *   - prepare unused index into ring buffer (initially put all indexes)
	 */
	dup_check() : _vect_pool(), _ring_vecant_idx(N_ENTRIES), _mmap_entries() {
		// create all entries in vector pool (gentry bodies) and ring buffer (unused item).
		_vect_pool.resize(N_ENTRIES);
		for(unsigned i = 0; i < _vect_pool.capacity(); i++) {
			_vect_pool[i]._u8_vect_idx = i; // save index of pool.
			_ring_vecant_idx.push_back() = i;  // push them.
		}
	}


	/**
	 * @brief check if the node is duplicated.
	 *   Find the node by given parameter and check if a same entry is in the mmap.
	 *   If found the same node, returns as `duplicated: true', otherwise put this
	 *   information into mmap.
	 *
	 *   The rejection algorithm is set by the template parameter (MODE).
	 *
	 * @param u32ser         32bit Serial number
	 * @param u16value       item value (node seq number or etc.)
	 * @param u32_timestamp  ms time count when received
	 * @return true          duplicated (should reject the node)
	 * @return false         determined as a new node.
	 */
	bool check_dup(uint32_t u32ser, uint16_t u16val, uint32_t u32_timestamp) {
		// find entry by key:u32ser.
		auto r = _mmap_entries.equal_range(u32ser);

		// MODE1: reject items with same item value.
		if ((MODE & 0xF0000) == MODE_REJECT_SAME_SEQ) {
			for(auto p = r.first; p != r.second; ++p) {
				if (p->_u16_value == u16val) {
					return true;
				}
			}
		} else
		// MODE2: reject items with older item value.
		if ((MODE & 0xF0000) == MODE_REJECT_OLDER_SEQ) {

			static const uint8_t _BITS = MODE & 0xF;
			static const uint8_t BITS = _BITS == 0 ? 16 : _BITS;

			// same id shall not be registered.
			bool b_accept = false;
			auto p = r.first;
			if (p != r.second) {
				int16_t diff = (u16val - p->_u16_value) << (16 - BITS);

				if (diff > 0) { // accept
					// update the value
					p->_u16_value = u16val;
					p->_u8_ts = TS_TOu8(u32_timestamp);
					b_accept = true;
				} else {
					b_accept = false;
				}

				// may not necessary (remove other nodes with same key)
				while(++p != r.second) _delete_entry(*p);

				return !b_accept;
			}
		}

		// not found, add this entry newly.
		if (!_insert_entry(u32ser, u16val, u32_timestamp)) {
			// failed to add it. in this case, it's treated as "duplicated."
			return true;
		}

		// false means "not duplicated, should accept the packet".
		return false;
	}

	/**
	 * alias to check_dup()
	 */
	bool operator() (uint32_t u32ser, uint16_t u16val, uint32_t u32_timestamp) {
		return check_dup(u32ser, u16val, u32_timestamp);
	}

	/**
	 * @brief remove older (timeout) nodes.
	 *   Remove nodes which is older than time out value from the given time stamp.
	 *
	 * @param u32now     the current timestamp.
	 */
	void clean_by_timeout(uint32_t u32now) {
		for(auto& x : _mmap_entries) {
			if (TS_DIFF(u32now, x._u8_ts) > TIMEOUT_ms) {
				_delete_entry(x);
			}
		}
	}

	/**
	 * @brief  reference to mmap container. (for debugging)
	 * @return internal reference to mmap data entry.
	 */
	const tmap &_entries() const { return _mmap_entries; }
};

#endif /* SOURCE_DUP_CHECKER_HPP_ */
