/**  
 * Copyright (c) 2009 Carnegie Mellon University. 
 *     All rights reserved.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an "AS
 *  IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 *  express or implied.  See the License for the specific language
 *  governing permissions and limitations under the License.
 *
 * For more about this software visit:
 *
 *      http://www.graphlab.ml.cmu.edu
 *
 */

#ifndef GRAPHLAB_UTIL_CUCKOO_MAP_POW2_HPP
#define GRAPHLAB_UTIL_CUCKOO_MAP_POW2_HPP

#include <vector>
#include <iterator>
#include <boost/random.hpp>
#include <boost/unordered_map.hpp>
#include <ctime>
#include<iostream>
#include<vector>
#include<string.h>
#include <assert.h>




  /**
   * A cuckoo hash map which requires the user to
   * provide an "illegal" value thus avoiding the need
   * for a seperate bitmap. More or less similar
   * interface as boost::unordered_map, not necessarily
   * entirely STL compliant.
   */
  template <typename Key, typename Value,
            size_t CuckooK = 3,
            typename IndexType = size_t,
            typename Hash = boost::hash<Key>,
            typename Pred = std::equal_to<Key> >
  class cuckoo_map_pow2 {

  public:
    // public typedefs
    typedef Key                                      key_type;
    typedef std::pair<Key const, Value>              value_type;
    typedef Value                                    mapped_type;
    typedef Hash                                     hasher;
    typedef Pred                                     key_equal;
    typedef IndexType                                index_type;
    typedef value_type* pointer;
    typedef value_type& reference;
    typedef const value_type* const_pointer;
    typedef const value_type& const_reference;

  private:
    // internal typedefs
    typedef std::pair<key_type, mapped_type> non_const_value_type;
    typedef value_type* map_container_type;
    typedef value_type* map_container_iterator;
    typedef const value_type* map_container_const_iterator;
    typedef boost::unordered_map<Key, Value, Hash, Pred> stash_container_type;

    key_type illegalkey;
    index_type numel;
    index_type maxstash;
    map_container_type data;
    size_t datalen;
    stash_container_type stash;
    boost::rand48  drng;
    boost::uniform_int<index_type> kranddist;
    hasher hashfun;
    key_equal keyeq;
    index_type mask;

    map_container_iterator data_begin() {
      return data;
    }

    map_container_iterator data_end() {
      return data + datalen;
    }

    map_container_const_iterator data_begin() const {
      return data;
    }

    map_container_const_iterator data_end() const {
      return data + datalen;
    }


    // bypass the const key_type with a placement new
    void replace_in_vector(map_container_iterator iter,
                           const key_type& key,
                           const mapped_type& val) {
      // delete
      iter->~value_type();
      // placement new
      new(iter) value_type(key, val);
    }

    void destroy_all() {
      // call ze destructors
      for(size_t i = 0; i < datalen; ++i) {
        data[i].~value_type();
      }
      free(data);
      stash.clear();
      data = NULL;
      datalen = 0;
      numel = 0;
    }

  public:
    struct insert_iterator{
      cuckoo_map_pow2* cmap;
      typedef std::forward_iterator_tag iterator_category;
      typedef typename cuckoo_map_pow2::value_type value_type;

      insert_iterator(cuckoo_map_pow2* c):cmap(c) {}
      
      insert_iterator operator++() {
        return (*this);
      }
      insert_iterator operator++(int) {
        return (*this);
      }

      insert_iterator& operator*() {
        return *this;
      }
      insert_iterator& operator=(const insert_iterator& i) {
        cmap = i.cmap;
        return *this;
      }
      
      insert_iterator& operator=(const value_type& v) {
        cmap->insert(v);
        return *this;
      }
    };

    struct const_iterator {
      const cuckoo_map_pow2* cmap;
      bool in_stash;
      typename cuckoo_map_pow2::map_container_const_iterator vec_iter;
      typename cuckoo_map_pow2::stash_container_type::const_iterator stash_iter;

      typedef std::forward_iterator_tag iterator_category;
      typedef typename cuckoo_map_pow2::value_type value_type;
      typedef size_t difference_type;
      typedef const value_type* pointer;
      typedef const value_type& reference;

      friend class cuckoo_map_pow2;

      const_iterator(): cmap(NULL), in_stash(false) {}

      const_iterator operator++() {
        if (!in_stash) {
          ++vec_iter;
          // we are in the main vector. try to advance the
          // iterator until I hit another data element
          while(vec_iter != cmap->data_end() &&
                cmap->key_eq()(vec_iter->first, cmap->illegal_key())) ++vec_iter;
          if (vec_iter == cmap->data_end()) {
            in_stash = true;
            stash_iter = cmap->stash.begin();
          }
        }
        else if (in_stash) {
          if (stash_iter != cmap->stash.end())  ++stash_iter;
        }
        return *this;
      }

      const_iterator operator++(int) {
        const_iterator cur = *this;
        ++(*this);
        return cur;
      }


      reference operator*() {
        if (!in_stash) return *vec_iter;
        else return *stash_iter;
      }

      pointer operator->() {
        if (!in_stash) return &(*vec_iter);
        else return &(*stash_iter);
      }

      bool operator==(const const_iterator iter) const {
        return in_stash == iter.in_stash &&
          (in_stash==false ?
           vec_iter == iter.vec_iter :
           stash_iter == iter.stash_iter);
      }

      bool operator!=(const const_iterator iter) const {
        return !((*this) == iter);
      }

    private:
      const_iterator(const cuckoo_map_pow2* cmap, typename cuckoo_map_pow2::map_container_const_iterator vec_iter):
        cmap(cmap), in_stash(false), vec_iter(vec_iter), stash_iter(cmap->stash.begin()) { }

      const_iterator(const cuckoo_map_pow2* cmap, typename cuckoo_map_pow2::stash_container_type::const_iterator stash_iter):
        cmap(cmap), in_stash(true), vec_iter(cmap->data_begin()), stash_iter(stash_iter) { }
      
    };


    struct iterator {
      cuckoo_map_pow2* cmap;
      bool in_stash;
      typename cuckoo_map_pow2::map_container_iterator vec_iter;
      typename cuckoo_map_pow2::stash_container_type::iterator stash_iter;

      typedef std::forward_iterator_tag iterator_category;
      typedef typename cuckoo_map_pow2::value_type value_type;
      typedef size_t difference_type;
      typedef value_type* pointer;
      typedef value_type& reference;

      friend class cuckoo_map_pow2;

      iterator(): cmap(NULL), in_stash(false) {}


      operator const_iterator() const {
        const_iterator iter;
        iter.cmap = cmap;
        iter.in_stash = in_stash;
        iter.vec_iter = vec_iter;
        iter.stash_iter = stash_iter;
        return iter;
      }

      iterator operator++() {
        if (!in_stash) {
          ++vec_iter;
          // we are in the main vector. try to advance the
          // iterator until I hit another data element
          while(vec_iter != cmap->data_end() &&
                cmap->key_eq()(vec_iter->first, cmap->illegal_key())) ++vec_iter;
          if (vec_iter == cmap->data_end()) {
            in_stash = true;
            stash_iter = cmap->stash.begin();
          }
        }
        else if (in_stash) {
          if (stash_iter != cmap->stash.end())  ++stash_iter;
        }
        return *this;
      }

      iterator operator++(int) {
        iterator cur = *this;
        ++(*this);
        return cur;
      }


      reference operator*() {
        if (!in_stash) return *vec_iter;
        else return *stash_iter;
      }

      pointer operator->() {
        if (!in_stash) return &(*vec_iter);
        else return &(*stash_iter);
      }

      bool operator==(const iterator iter) const {
        return in_stash == iter.in_stash &&
          (in_stash==false ?
           vec_iter == iter.vec_iter :
           stash_iter == iter.stash_iter);
      }

      bool operator!=(const iterator iter) const {
        return !((*this) == iter);
      }


    private:
      iterator(cuckoo_map_pow2* cmap, 
               typename cuckoo_map_pow2::map_container_iterator vec_iter):
        cmap(cmap), in_stash(false), vec_iter(vec_iter) { }

      iterator(cuckoo_map_pow2* cmap, 
               typename cuckoo_map_pow2::stash_container_type::iterator stash_iter):
        cmap(cmap), in_stash(true), stash_iter(stash_iter) { }

    };


  private:

    // the primary inserting logic.
    // this assumes that the data is not already in the array.
    // caller must check before performing the insert
    iterator do_insert(const value_type& v_) {
      non_const_value_type v(v_.first, v_.second);
      if (stash.size() > maxstash) {
        // resize
        reserve(datalen * 2);
      }

      index_type insertpos = (index_type)(-1); // tracks where the current
      // inserted value went
      ++numel;

      // take a random walk down the tree
      for (int i = 0;i < 100; ++i) {
        // first see if one of the hashes will work
        index_type idx = 0;
        bool found = false;
        size_t hash_of_k = hashfun(v.first);
        for (size_t j = 0; j < CuckooK; ++j) {
          idx = compute_hash(hash_of_k, j);
          if (keyeq(data[idx].first, illegalkey)) {
            found = true;
            break;
          }
        }
        if (!found) idx = compute_hash(hash_of_k, kranddist(drng));
        // if insertpos is -1, v holds the current value. and we
        //                     are inserting it into idx
        // if insertpos is idx, we are bumping v again. and v will hold the
        //                      current value once more. so revert
        //                      insertpos to -1
        if (insertpos == (index_type)(-1)) insertpos = idx;
        else if (insertpos == idx) insertpos = (index_type)(-1);
        // there is room here
        if (found || keyeq(data[idx].first, illegalkey)) {
          replace_in_vector(data_begin() + idx, v.first, v.second);
          // success!
          return iterator(this, data_begin() + insertpos);
        }
        // failed to insert!
        // try again!

        non_const_value_type tmp = data[idx];
        replace_in_vector(data_begin() + idx, v.first, v.second);
        v = tmp;
      }
      // ok. tried and failed 100 times.
      //stick it in the stash

      typename stash_container_type::iterator stashiter = stash.insert(v).first;
      // if insertpos is -1, current value went into stash
      if (insertpos == (index_type)(-1)) {
        return iterator(this, stashiter);
      }
      else {
        return iterator(this, data_begin() + insertpos);
      }
    }
  public:

    cuckoo_map_pow2(key_type illegalkey,
                    index_type stashsize = 8,
                    hasher const& h = hasher(),
                    key_equal const& k = key_equal()):
      illegalkey(illegalkey),
      numel(0),maxstash(stashsize),
      data(NULL), datalen(0),
      drng(time(NULL)),
      kranddist(0, CuckooK - 1), hashfun(h), keyeq(k), mask(127) {
      stash.max_load_factor(1.0);
      reserve(128);
    }

    const key_type& illegal_key() const {
      return illegalkey;
    }

    ~cuckoo_map_pow2() {
      destroy_all();
    }

    cuckoo_map_pow2& operator=(const cuckoo_map_pow2& other) {
      destroy_all();
      // copy the data
      data = (map_container_type)malloc(sizeof(value_type) * other.datalen);
      datalen = other.datalen;
      std::uninitialized_copy(other.data_begin(), other.data_end(), data_begin());

      // copy the stash
      stash = other.stash;

      // copy all the other extra stuff
      illegalkey = other.illegalkey;
      numel = other.numel;
      hashfun = other.hashfun;
      keyeq = other.keyeq;
      mask = other.mask;
      return *this;
    }
  
    index_type size() {
      return numel;
    }

    iterator begin() {
      iterator iter;
      iter.cmap = this;
      iter.in_stash = false;
      iter.vec_iter = data_begin();

      while(iter.vec_iter != data_end() &&
            keyeq(iter.vec_iter->first, illegalkey)) ++iter.vec_iter;

      if (iter.vec_iter == data_end()) {
        iter.in_stash = true;
        iter.stash_iter = stash.begin();
      }
      return iter;
    }

    iterator end() {
      return iterator(this, stash.end());
    }


    const_iterator begin() const {
      const_iterator iter;
      iter.cmap = this;
      iter.in_stash = false;
      iter.vec_iter = data_begin();

      while(iter.vec_iter != data_end() &&
            keyeq(iter.vec_iter->first, illegalkey)) ++iter.vec_iter;

      if (iter.vec_iter == data_end()) {
        iter.in_stash = true;
        iter.stash_iter = stash.begin();
      }

      return iter;
    }

    const_iterator end() const {
      return const_iterator(this, stash.end());

    }

    /*
     * Bob Jenkin's 32 bit integer mix function from
     * http://home.comcast.net/~bretm/hash/3.html
     */
    static size_t mix(size_t state) {
      state += (state << 12);
      state ^= (state >> 22);
      state += (state << 4);
      state ^= (state >> 9);
      state += (state << 10);
      state ^= (state >> 2);
      state += (state << 7);
      state ^= (state >> 12);
      return state;
    }

    index_type compute_hash(size_t k , const uint32_t seed) const {
      // a bunch of random numbers
#if (__SIZEOF_PTRDIFF_T__ == 8)
      static const size_t a[8] = {0x6306AA9DFC13C8E7,
                                  0xA8CD7FBCA2A9FFD4,
                                  0x40D341EB597ECDDC,
                                  0x99CFA1168AF8DA7E,
                                  0x7C55BCC3AF531D42,
                                  0x1BC49DB0842A21DD,
                                  0x2181F03B1DEE299F,
                                  0xD524D92CBFEC63E9};
#else
      static const size_t a[8] = {0xFC13C8E7,
                                  0xA2A9FFD4,
                                  0x597ECDDC,
                                  0x8AF8DA7E,
                                  0xAF531D42,
                                  0x842A21DD,
                                  0x1DEE299F,
                                  0xBFEC63E9};
#endif
      index_type s = mix(a[seed] ^ k);
      return s & mask;
    }

    void rehash() {
      stash_container_type stmp;
      stmp.swap(stash);
      // effectively, stmp elements are deleted
      numel -= stmp.size();
      for (size_t i = 0;i < datalen; ++i) {
        // if there is an element here. erase it and reinsert
        if (!keyeq(data[i].first, illegalkey)) {
          if (count(data[i].first)) continue;
          non_const_value_type v = data[i];
          replace_in_vector(data_begin() + i, illegalkey, mapped_type());
          numel--;
          //erase(iterator(this, data_begin() + i));
          insert(v);
        }
      }
      typename stash_container_type::const_iterator iter = stmp.begin();
      while(iter != stmp.end()) {
        insert(*iter);
        ++iter;
      }
    }

    static uint64_t next_powerof2(uint64_t val) {
      --val;
      val = val | (val >> 1);
      val = val | (val >> 2);
      val = val | (val >> 4);
      val = val | (val >> 8);
      val = val | (val >> 16);
      val = val | (val >> 32);
      return val + 1;
    }

  
    void reserve(size_t newlen) {
      newlen = next_powerof2(newlen);
      if (newlen <= datalen) return;
      mask = newlen - 1;
      //data.reserve(newlen);
      //data.resize(newlen, std::make_pair<Key, Value>(illegalkey, Value()));
      data = (map_container_type)realloc(data, newlen * sizeof(value_type));
      std::uninitialized_fill(data_end(), data+newlen, non_const_value_type(illegalkey, mapped_type()));
      datalen = newlen;
      rehash();
    }

    std::pair<iterator, bool> insert(const value_type& v_) {
      iterator i = find(v_.first);
      if (i != end()) return std::make_pair(i, false);
      else return std::make_pair(do_insert(v_), true);
    }

    iterator insert(const_iterator hint, value_type const& v) {
      return insert(v).first;
    }

    iterator find(key_type const& k) {
      size_t hash_of_k = hashfun(k);
      for (uint32_t i = 0;i < CuckooK; ++i) {
        index_type idx = compute_hash(hash_of_k, i);
        if (keyeq(data[idx].first, k)) return iterator(this, data_begin() + idx);
      }
      return iterator(this, stash.find(k));
    }

    const_iterator find(key_type const& k) const {
      size_t hash_of_k = hashfun(k);
      for (uint32_t i = 0;i < CuckooK; ++i) {
        index_type idx = compute_hash(hash_of_k, i);
        if (keyeq(data[idx].first, k)) return const_iterator(this, data_begin() + idx);
      }
      return const_iterator(this, stash.find(k));
    }

    size_t count(key_type const& k) const {
      size_t hash_of_k = hashfun(k);
      for (uint32_t i = 0;i < CuckooK; ++i) {
        index_type idx = compute_hash(hash_of_k, i);
        if (keyeq(data[idx].first, k)) return true;
      }
      return stash.count(k);
    }

  
    void erase(iterator iter) {
      if (iter.in_stash == false) {
        if (!keyeq(iter.vec_iter->first, illegalkey)) {
        
          replace_in_vector(&(*(iter.vec_iter)), illegalkey, mapped_type());

          --numel;
        }
      }
      else if (iter.stash_iter != stash.end()) {
        --numel;
        stash.erase(iter.stash_iter);
      }
    }

    void erase(key_type const& k) {
      iterator iter = find(k);
      if (iter != end()) erase(iter);
    }

    void swap(cuckoo_map_pow2& other) {
      std::swap(illegalkey, other.illegalkey);
      std::swap(numel, other.numel);
      std::swap(maxstash, other.maxstash);
      std::swap(data, other.data);
      std::swap(datalen, other.datalen);
      std::swap(stash, other.stash);
      std::swap(drng, other.drng);
      std::swap(kranddist, other.kranddist);
      std::swap(hashfun, other.hashfun);
      std::swap(keyeq, other.keyeq);
      std::swap(mask, other.mask);
    }
  
    mapped_type& operator[](const key_type& i) {
      iterator iter = find(i);
      value_type tmp(i, mapped_type());
      if (iter == end()) iter = do_insert(tmp);
      return iter->second;
    }

    key_equal key_eq() const {
      return keyeq;
    }

    void clear() {
      destroy_all();
      reserve(128);
    }


    float load_factor() const {
      return (float)numel / (datalen + stash.size());
    }

    // void save(oarchive &oarc) const {
    //   oarc << numel << illegalkey;
    //   serialize_iterator(oarc, begin(), end(), numel);
    // }


    // void load(iarchive &iarc) {
    //   clear();
    //   index_type tmpnumel = 0;
    //   iarc >> tmpnumel >> illegalkey;
    //   //std::cout << tmpnumel << ", " << illegalkey << std::endl;
    //   reserve(tmpnumel * 1.5);
    //   deserialize_iterator<iarchive, non_const_value_type>
    //     (iarc, insert_iterator(this));
    //   // for(size_t i = 0; i < tmpnumel; ++i) {
    //   //   non_const_value_type pair;
    //   //   iarc >> pair; 
    //   //   operator[](pair.first) = pair.second;
    //   // }
    // }
  
  }; // end of cuckoo_map_pow2

#endif



#define RPC_MAX_N_PROCS 8
using namespace std;

template <int len>
  class fixed_dense_bitset {
  public:
    /// Constructs a bitset of 0 length
    fixed_dense_bitset() {
      clear();
    }
    
   /// Make a copy of the bitset db
    fixed_dense_bitset(const fixed_dense_bitset<len> &db) {
      *this = db;
    }

    /** Initialize this fixed dense bitset by copying 
        ceil(len/(wordlen)) words from mem
    */
    void initialize_from_mem(void* mem, size_t memlen) {
      memcpy(array, mem, memlen);
    }
    
    /// destructor
    ~fixed_dense_bitset() {}
  
    /// Make a copy of the bitset db
    inline fixed_dense_bitset<len>& operator=(const fixed_dense_bitset<len>& db) {
      memcpy(array, db.array, sizeof(size_t) * arrlen);
      return *this;
    }
  
    /// Sets all bits to 0
    inline void clear() {
      memset((void*)array, 0, sizeof(size_t) * arrlen);
    }
    
    /// Sets all bits to 1
    inline void fill() {
      for (size_t i = 0;i < arrlen; ++i) array[i] = -1;
      fix_trailing_bits();
    }

    inline bool empty() const {
      for (size_t i = 0; i < arrlen; ++i) if (array[i]) return false;
      return true;
    }
    
    /// Prefetches the word containing the bit b
    inline void prefetch(size_t b) const{
      __builtin_prefetch(&(array[b / (8 * sizeof(size_t))]));
    }
    
    /// Returns the value of the bit b
    inline bool get(size_t b) const{
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      return array[arrpos] & (size_t(1) << size_t(bitpos));
    }

    //! Atomically sets the bit at b to true returning the old value
    inline bool set_bit(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t mask(size_t(1) << size_t(bitpos)); 
      return __sync_fetch_and_or(array + arrpos, mask) & mask;
    }


    //! Returns the value of the word containing the bit b 
    inline size_t containing_word(size_t b) {
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      return array[arrpos];
    }


    /** Set the bit at position b to true returning the old value.
        Unlike set_bit(), this uses a non-atomic set which is faster,
        but is unsafe if accessed by multiple threads.
    */
    inline bool set_bit_unsync(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t mask(size_t(1) << size_t(bitpos)); 
      bool ret = array[arrpos] & mask;
      array[arrpos] |= mask;
      return ret;
    }

    /** Set the state of the bit returning the old value.
      This version uses a non-atomic set which is faster, but
      is unsafe if accessed by multiple threads.
    */
    inline bool set(size_t b, bool value) {
      if (value) return set_bit(b);
      else return clear_bit(b);
    }

    /** Set the state of the bit returning the old value.
      This version uses a non-atomic set which is faster, but
      is unsafe if accessed by multiple threads.
    */
    inline bool set_unsync(size_t b, bool value) {
      if (value) return set_bit_unsync(b);
      else return clear_bit_unsync(b);
    }


    //! Atomically set the bit at b to false returning the old value
    inline bool clear_bit(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t test_mask(size_t(1) << size_t(bitpos)); 
      const size_t clear_mask(~test_mask); 
      return __sync_fetch_and_and(array + arrpos, clear_mask) & test_mask;
    }

    /** Clears the state of the bit returning the old value.
      This version uses a non-atomic set which is faster, but
      is unsafe if accessed by multiple threads.
    */
    inline bool clear_bit_unsync(size_t b) {
      // use CAS to set the bit
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      const size_t test_mask(size_t(1) << size_t(bitpos)); 
      const size_t clear_mask(~test_mask); 
      bool ret = array[arrpos] & test_mask;
      array[arrpos] &= clear_mask;
      return ret;
    }


    struct bit_pos_iterator {
      typedef std::input_iterator_tag iterator_category;
      typedef size_t value_type;
      typedef size_t difference_type;
      typedef const size_t reference;
      typedef const size_t* pointer;
      size_t pos;
      const fixed_dense_bitset* db;
      bit_pos_iterator():pos(-1),db(NULL) {}
      bit_pos_iterator(const fixed_dense_bitset* const db, size_t pos):pos(pos),db(db) {}
      
      size_t operator*() const {
        return pos;
      }
      size_t operator++(){
        if (db->next_bit(pos) == false) pos = (size_t)(-1);
        return pos;
      }
      size_t operator++(int){
        size_t prevpos = pos;
        if (db->next_bit(pos) == false) pos = (size_t)(-1);
        return prevpos;
      }
      bool operator==(const bit_pos_iterator& other) const {
        ASSERT_TRUE(db == other.db);
        return other.pos == pos;
      }
      bool operator!=(const bit_pos_iterator& other) const {
        ASSERT_TRUE(db == other.db);
        return other.pos != pos;
      }
    };
    
    typedef bit_pos_iterator iterator;
    typedef bit_pos_iterator const_iterator;

    
    bit_pos_iterator begin() const {
      size_t pos;
      if (first_bit(pos) == false) pos = size_t(-1);
      return bit_pos_iterator(this, pos);
    }
    
    bit_pos_iterator end() const {
      return bit_pos_iterator(this, (size_t)(-1));
    }

    /** Returns true with b containing the position of the 
        first bit set to true.
        If such a bit does not exist, this function returns false.
    */
    inline bool first_bit(size_t &b) const {
      for (size_t i = 0; i < arrlen; ++i) {
        if (array[i]) {
          b = (size_t)(i * (sizeof(size_t) * 8)) + first_bit_in_block(array[i]);
          return true;
        }
      }
      return false;
    }

    /** Returns true with b containing the position of the 
        first bit set to false.
        If such a bit does not exist, this function returns false.
    */
    inline bool first_zero_bit(size_t &b) const {
      for (size_t i = 0; i < arrlen; ++i) {
        if (~array[i]) {
          b = (size_t)(i * (sizeof(size_t) * 8)) + first_bit_in_block(~array[i]);
          return true;
        }
      }
      return false;
    }



    /** Where b is a bit index, this function will return in b,
        the position of the next bit set to true, and return true.
        If all bits after b are false, this function returns false.
    */
    inline bool next_bit(size_t &b) const {
      size_t arrpos, bitpos;
      bit_to_pos(b, arrpos, bitpos);
      //try to find the next bit in this block
      bitpos = next_bit_in_block(bitpos, array[arrpos]);
      if (bitpos != 0) {
        b = (size_t)(arrpos * (sizeof(size_t) * 8)) + bitpos;
        return true;
      }
      else {
        // we have to loop through the rest of the array
        for (size_t i = arrpos + 1; i < arrlen; ++i) {
          if (array[i]) {
            b = (size_t)(i * (sizeof(size_t) * 8)) + first_bit_in_block(array[i]);
            return true;
          }
        }
      }
      return false;
    }
    
    ///  Returns the number of bits in this bitset
    inline size_t size() const {
      return len;
    }
    
    /// Serializes this bitset to an archive
    // inline void save(oarchive& oarc) const {
    //   //oarc <<len << arrlen;
    //   //if (arrlen > 0)
    //   serialize(oarc, array, arrlen*sizeof(size_t));
    // }

    /// Deserializes this bitset from an archive
    // inline void load(iarchive& iarc) {
    //   /*size_t l;
    //   size_t arl;
    //   iarc >> l >> arl;
    //   ASSERT_EQ(l, len);
    //   ASSERT_EQ(arl, arrlen);*/
    //   //if (arrlen > 0) {
    //   deserialize(iarc, array, arrlen*sizeof(size_t));
    //   //}
    // }

    size_t popcount() const {
      size_t ret = 0;
      for (size_t i = 0;i < arrlen; ++i) {
        ret +=  __builtin_popcountl(array[i]);
      }
      return ret;
    }

    // fixed_dense_bitset operator&(const fixed_dense_bitset& other) const {
    //   ASSERT_EQ(size(), other.size());
    //   fixed_dense_bitset ret(size());
    //   for (size_t i = 0; i < arrlen; ++i) {
    //     ret.array[i] = array[i] & other.array[i];
    //   }
    //   return ret;
    // }


    // fixed_dense_bitset operator|(const fixed_dense_bitset& other) const {
    //   ASSERT_EQ(size(), other.size());
    //   fixed_dense_bitset ret(size());
    //   for (size_t i = 0; i < arrlen; ++i) {
    //     ret.array[i] = array[i] | other.array[i];
    //   }
    //   return ret;
    // }

    // fixed_dense_bitset operator-(const fixed_dense_bitset& other) const {
    //   ASSERT_EQ(size(), other.size());
    //   fixed_dense_bitset ret(size());
    //   for (size_t i = 0; i < arrlen; ++i) {
    //     ret.array[i] = array[i] - (array[i] & other.array[i]);
    //   }
    //   return ret;
    // }


    // fixed_dense_bitset& operator&=(const fixed_dense_bitset& other) {
    //   ASSERT_EQ(size(), other.size());
    //   for (size_t i = 0; i < arrlen; ++i) {
    //     array[i] &= other.array[i];
    //   }
    //   return *this;
    // }

  private:
    inline static void bit_to_pos(size_t b, size_t &arrpos, size_t &bitpos) {
      // the compiler better optimize this...
      arrpos = b / (8 * sizeof(size_t));
      bitpos = b & (8 * sizeof(size_t) - 1);
    }
  

    // returns 0 on failure
    inline size_t next_bit_in_block(const size_t &b, const size_t &block) const {
      size_t belowselectedbit = size_t(-1) - (((size_t(1) << b) - 1)|(size_t(1)<<b));
      size_t x = block & belowselectedbit ;
      if (x == 0) return 0;
      else return (size_t)__builtin_ctzl(x);
    }

    // returns 0 on failure
    inline size_t first_bit_in_block(const size_t &block) const {
      // use CAS to set the bit
      if (block == 0) return 0;
      else return (size_t)__builtin_ctzl(block);
    }

    // clears the trailing bits in the last block which are not part
    // of the actual length of the bitset
    void fix_trailing_bits() {
      // how many bits are in the last block
      size_t lastbits = len % (8 * sizeof(size_t));
      if (lastbits == 0) return;
      array[arrlen - 1] &= ((size_t(1) << lastbits) - 1);
    }

 
    static const size_t arrlen;
    size_t array[len / (sizeof(size_t) * 8) + (len % (sizeof(size_t) * 8) > 0)];
  };
  template<int len>
  const size_t fixed_dense_bitset<len>::arrlen = len / (sizeof(size_t) * 8) + (len % (sizeof(size_t) * 8) > 0);

inline uint32_t integer_mix(uint32_t a) {
  a -= (a<<6);
  a ^= (a>>17);
  a -= (a<<9);
  a ^= (a<<4);
  a -= (a<<3);
  a ^= (a<<10);
  a ^= (a>>15);
  return a;
}

namespace graph_hash {
    /** \brief Returns the hashed value of a vertex. */
    inline static size_t hash_vertex (const int vid) { 
      return integer_mix(vid);
    }

    /** \brief Returns the hashed value of an edge. */
    inline static size_t hash_edge (const std::pair<int, int>& e, const uint32_t seed = 5) {
      // a bunch of random numbers
#if (__SIZEOF_PTRDIFF_T__ == 8)
      static const size_t a[8] = {0x6306AA9DFC13C8E7,
        0xA8CD7FBCA2A9FFD4,
        0x40D341EB597ECDDC,
        0x99CFA1168AF8DA7E,
        0x7C55BCC3AF531D42,
        0x1BC49DB0842A21DD,
        0x2181F03B1DEE299F,
        0xD524D92CBFEC63E9};
#else
      static const size_t a[8] = {0xFC13C8E7,
        0xA2A9FFD4,
        0x597ECDDC,
        0x8AF8DA7E,
        0xAF531D42,
        0x842A21DD,
        0x1DEE299F,
        0xBFEC63E9};
#endif
      int src = e.first;
      int dst = e.second;
      return (integer_mix(src^a[seed%8]))^(integer_mix(dst^a[(seed+1)%8]));
    }
} // end of graph_hash namespace

