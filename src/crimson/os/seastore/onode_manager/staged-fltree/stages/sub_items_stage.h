// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:nil -*-
// vim: ts=8 sw=2 smarttab

#pragma once

#include <variant>

#include "crimson/os/seastore/onode_manager/staged-fltree/node_types.h"
#include "key_layout.h"
#include "stage_types.h"

namespace crimson::os::seastore::onode {

class NodeExtentMutable;

struct internal_sub_item_t {
  const snap_gen_t& get_key() const { return key; }
  const laddr_packed_t* get_p_value() const { return &value; }

  snap_gen_t key;
  laddr_packed_t value;
} __attribute__((packed));

/**
 * internal_sub_items_t
 *
 * The STAGE_RIGHT implementation for internal node N0/N1/N2, implements staged
 * contract as an indexable container to index snap-gen to child node
 * addresses.
 *
 * The layout of the contaner storing n sub-items:
 *
 * # <--------- container range -----------> #
 * #<~># sub-items [2, n)                    #
 * #   # <- sub-item 1 -> # <- sub-item 0 -> #
 * #...# snap-gen | laddr # snap-gen | laddr #
 *                        ^
 *                        |
 *           p_first_item +
 */
class internal_sub_items_t {
 public:
  using num_keys_t = index_t;

  internal_sub_items_t(const memory_range_t& range) {
    assert(range.p_start < range.p_end);
    assert((range.p_end - range.p_start) % sizeof(internal_sub_item_t) == 0);
    num_items = (range.p_end - range.p_start) / sizeof(internal_sub_item_t);
    assert(num_items > 0);
    auto _p_first_item = range.p_end - sizeof(internal_sub_item_t);
    p_first_item = reinterpret_cast<const internal_sub_item_t*>(_p_first_item);
  }

  // container type system
  using key_get_type = const snap_gen_t&;
  static constexpr auto CONTAINER_TYPE = ContainerType::INDEXABLE;
  num_keys_t keys() const { return num_items; }
  key_get_type operator[](index_t index) const {
    assert(index < num_items);
    return (p_first_item - index)->get_key();
  }
  node_offset_t size_before(index_t index) const {
    size_t ret = index * sizeof(internal_sub_item_t);
    assert(ret < NODE_BLOCK_SIZE);
    return ret;
  }
  const laddr_packed_t* get_p_value(index_t index) const {
    assert(index < num_items);
    return (p_first_item - index)->get_p_value();
  }
  node_offset_t size_overhead_at(index_t index) const { return 0u; }
  void encode(const char* p_node_start, stone::bufferlist& encoded) const {
    auto p_end = reinterpret_cast<const char*>(p_first_item) +
                 sizeof(internal_sub_item_t);
    auto p_start = p_end - num_items * sizeof(internal_sub_item_t);
    int start_offset = p_start - p_node_start;
    int end_offset = p_end - p_node_start;
    assert(start_offset > 0 &&
           start_offset < end_offset &&
           end_offset < NODE_BLOCK_SIZE);
    stone::encode(static_cast<node_offset_t>(start_offset), encoded);
    stone::encode(static_cast<node_offset_t>(end_offset), encoded);
  }

  static internal_sub_items_t decode(
      const char* p_node_start, stone::bufferlist::const_iterator& delta) {
    node_offset_t start_offset;
    stone::decode(start_offset, delta);
    node_offset_t end_offset;
    stone::decode(end_offset, delta);
    assert(start_offset < end_offset);
    assert(end_offset <= NODE_BLOCK_SIZE);
    return internal_sub_items_t({p_node_start + start_offset,
                                 p_node_start + end_offset});
  }

  static node_offset_t header_size() { return 0u; }

  template <KeyT KT>
  static node_offset_t estimate_insert(
      const full_key_t<KT>&, const laddr_packed_t&) {
    return sizeof(internal_sub_item_t);
  }

  template <KeyT KT>
  static const laddr_packed_t* insert_at(
      NodeExtentMutable&, const internal_sub_items_t&,
      const full_key_t<KT>&, const laddr_packed_t&,
      index_t index, node_offset_t size, const char* p_left_bound);

  static node_offset_t trim_until(NodeExtentMutable&, internal_sub_items_t&, index_t);

  template <KeyT KT>
  class Appender;

 private:
  index_t num_items;
  const internal_sub_item_t* p_first_item;
};

template <KeyT KT>
class internal_sub_items_t::Appender {
 public:
  Appender(NodeExtentMutable* p_mut, char* p_append)
    : p_mut{p_mut}, p_append{p_append} {}
  void append(const internal_sub_items_t& src, index_t from, index_t items);
  void append(const full_key_t<KT>&, const laddr_packed_t&, const laddr_packed_t*&);
  char* wrap() { return p_append; }
 private:
  NodeExtentMutable* p_mut;
  char* p_append;
};

/**
 * leaf_sub_items_t
 *
 * The STAGE_RIGHT implementation for leaf node N0/N1/N2, implements staged
 * contract as an indexable container to index snap-gen to onode_t.
 *
 * The layout of the contaner storing n sub-items:
 *
 * # <------------------------ container range -------------------------------> #
 * # <---------- sub-items ----------------> # <--- offsets ---------#          #
 * #<~># sub-items [2, n)                    #<~>| offsets [2, n)    #          #
 * #   # <- sub-item 1 -> # <- sub-item 0 -> #   |                   #          #
 * #...# snap-gen | onode # snap-gen | onode #...| offset1 | offset0 # num_keys #
 *                                           ^             ^         ^
 *                                           |             |         |
 *                               p_items_end +   p_offsets +         |
 *                                                        p_num_keys +
 */
class leaf_sub_items_t {
 public:
  // TODO: decide by NODE_BLOCK_SIZE, sizeof(snap_gen_t),
  //       and the minimal size of onode_t
  using num_keys_t = uint8_t;

  leaf_sub_items_t(const memory_range_t& range) {
    assert(range.p_start < range.p_end);
    auto _p_num_keys = range.p_end - sizeof(num_keys_t);
    assert(range.p_start < _p_num_keys);
    p_num_keys = reinterpret_cast<const num_keys_t*>(_p_num_keys);
    assert(keys());
    auto _p_offsets = _p_num_keys - sizeof(node_offset_t);
    assert(range.p_start < _p_offsets);
    p_offsets = reinterpret_cast<const node_offset_packed_t*>(_p_offsets);
    p_items_end = reinterpret_cast<const char*>(&get_offset(keys() - 1));
    assert(range.p_start < p_items_end);
    assert(range.p_start == p_start());
  }

  bool operator==(const leaf_sub_items_t& x) {
    return (p_num_keys == x.p_num_keys &&
            p_offsets == x.p_offsets &&
            p_items_end == x.p_items_end);
  }

  const char* p_start() const { return get_item_end(keys()); }

  const node_offset_packed_t& get_offset(index_t index) const {
    assert(index < keys());
    return *(p_offsets - index);
  }

  const node_offset_t get_offset_to_end(index_t index) const {
    assert(index <= keys());
    return index == 0 ? 0 : get_offset(index - 1).value;
  }

  const char* get_item_start(index_t index) const {
    return p_items_end - get_offset(index).value;
  }

  const char* get_item_end(index_t index) const {
    return p_items_end - get_offset_to_end(index);
  }

  // container type system
  using key_get_type = const snap_gen_t&;
  static constexpr auto CONTAINER_TYPE = ContainerType::INDEXABLE;
  num_keys_t keys() const { return *p_num_keys; }
  key_get_type operator[](index_t index) const {
    assert(index < keys());
    auto pointer = get_item_end(index);
    assert(get_item_start(index) < pointer);
    pointer -= sizeof(snap_gen_t);
    assert(get_item_start(index) < pointer);
    return *reinterpret_cast<const snap_gen_t*>(pointer);
  }
  node_offset_t size_before(index_t index) const {
    assert(index <= keys());
    size_t ret;
    if (index == 0) {
      ret = sizeof(num_keys_t);
    } else {
      --index;
      ret = sizeof(num_keys_t) +
            (index + 1) * sizeof(node_offset_t) +
            get_offset(index).value;
    }
    assert(ret < NODE_BLOCK_SIZE);
    return ret;
  }
  node_offset_t size_overhead_at(index_t index) const { return sizeof(node_offset_t); }
  const onode_t* get_p_value(index_t index) const {
    assert(index < keys());
    auto pointer = get_item_start(index);
    auto value = reinterpret_cast<const onode_t*>(pointer);
    assert(pointer + value->size + sizeof(snap_gen_t) == get_item_end(index));
    return value;
  }
  void encode(const char* p_node_start, stone::bufferlist& encoded) const {
    auto p_end = reinterpret_cast<const char*>(p_num_keys) +
                  sizeof(num_keys_t);
    int start_offset = p_start() - p_node_start;
    int end_offset = p_end - p_node_start;
    assert(start_offset > 0 &&
           start_offset < end_offset &&
           end_offset < NODE_BLOCK_SIZE);
    stone::encode(static_cast<node_offset_t>(start_offset), encoded);
    stone::encode(static_cast<node_offset_t>(end_offset), encoded);
  }

  static leaf_sub_items_t decode(
      const char* p_node_start, stone::bufferlist::const_iterator& delta) {
    node_offset_t start_offset;
    stone::decode(start_offset, delta);
    node_offset_t end_offset;
    stone::decode(end_offset, delta);
    assert(start_offset < end_offset);
    assert(end_offset <= NODE_BLOCK_SIZE);
    return leaf_sub_items_t({p_node_start + start_offset,
                             p_node_start + end_offset});
  }

  static node_offset_t header_size() { return sizeof(num_keys_t); }

  template <KeyT KT>
  static node_offset_t estimate_insert(const full_key_t<KT>&, const onode_t& value) {
    return value.size + sizeof(snap_gen_t) + sizeof(node_offset_t);
  }

  template <KeyT KT>
  static const onode_t* insert_at(
      NodeExtentMutable&, const leaf_sub_items_t&,
      const full_key_t<KT>&, const onode_t&,
      index_t index, node_offset_t size, const char* p_left_bound);

  static node_offset_t trim_until(NodeExtentMutable&, leaf_sub_items_t&, index_t index);

  template <KeyT KT>
  class Appender;

 private:
  // TODO: support unaligned access
  const num_keys_t* p_num_keys;
  const node_offset_packed_t* p_offsets;
  const char* p_items_end;
};

constexpr index_t APPENDER_LIMIT = 3u;

template <KeyT KT>
class leaf_sub_items_t::Appender {
  struct range_items_t {
    index_t from;
    index_t items;
  };
  struct kv_item_t {
    const full_key_t<KT>* p_key;
    const onode_t* p_value;
  };
  using var_t = std::variant<range_items_t, kv_item_t>;

 public:
  Appender(NodeExtentMutable* p_mut, char* p_append)
    : p_mut{p_mut}, p_append{p_append} {
  }

  void append(const leaf_sub_items_t& src, index_t from, index_t items) {
    assert(cnt <= APPENDER_LIMIT);
    assert(from <= src.keys());
    if (items == 0) {
      return;
    }
    if (op_src) {
      assert(*op_src == src);
    } else {
      op_src = src;
    }
    assert(from < src.keys());
    assert(from + items <= src.keys());
    appends[cnt] = range_items_t{from, items};
    ++cnt;
  }
  void append(const full_key_t<KT>& key,
              const onode_t& value, const onode_t*& p_value) {
    assert(pp_value == nullptr);
    assert(cnt <= APPENDER_LIMIT);
    appends[cnt] = kv_item_t{&key, &value};
    ++cnt;
    pp_value = &p_value;
  }
  char* wrap();

 private:
  std::optional<leaf_sub_items_t> op_src;
  const onode_t** pp_value = nullptr;
  NodeExtentMutable* p_mut;
  char* p_append;
  var_t appends[APPENDER_LIMIT];
  index_t cnt = 0;
};

template <node_type_t> struct _sub_items_t;
template<> struct _sub_items_t<node_type_t::INTERNAL> { using type = internal_sub_items_t; };
template<> struct _sub_items_t<node_type_t::LEAF> { using type = leaf_sub_items_t; };
template <node_type_t NODE_TYPE>
using sub_items_t = typename _sub_items_t<NODE_TYPE>::type;

}
