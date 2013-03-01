#ifndef _LUM_MAP_H_
#define _LUM_MAP_H_

#include <lum/common.h>
#include <lum/hash.h>
#include <lum/str.h>

namespace lum {

// Binary representation of integer `x` of length `bitcount`
// E.g:
//   std::cout << debug_fbin((uintmax_t)&debug_fbin, sizeof(void*)*8) << '\n';
//   00000000 00000000 00000000 00000001 00000100 10000100 10110010 10110000
//
inline const char* __attribute__((unused))
debug_fbin32(uint32_t a) {
  const size_t buflen = 9 * sizeof(uint32_t) + 1; // 9=8C+1SP
  static char buf[buflen] = {0};
  char* str = buf + buflen - 1;
  size_t i = 31;
  while (1) {
    *str = (a & 1) + '0';
    a >>= 1;
    if (i == 0) { break; }
    if ((i+3) % 5 == 0) { *--str = ' '; }
    --str;
    --i;
  }
  return str;
}

typedef uint32_t u32;

#if 1
static inline int LUM_ALWAYS_INLINE popcount_u32(u32 n) {
  return __builtin_popcount(n);
}
#else
/* Classic binary divide-and-conquer popcount.
   This is popcount_2() from
   http://en.wikipedia.org/wiki/Hamming_weight */
/* 15 ops, 3 long immediates, 14 stages */
static inline u32 popcount_u32(u32 x) {
  u32 m1 = 0x55555555;
  u32 m2 = 0x33333333;
  u32 m4 = 0x0f0f0f0f;
  x -= (x >> 1) & m1;
  x = (x & m2) + ((x >> 2) & m2);
  x = (x + (x >> 4)) & m4;
  x += x >>  8;
  return (x + (x >> 16)) & 0x3f;
}
#endif


template <typename KeyType, typename ValueType, u32(*KeyHash)(KeyType)>
struct Map {
  typedef Map<KeyType,ValueType,KeyHash>* MapType;
  struct Node;

  u32 count;
  Node* root;

  constexpr Map(u32 c, Node* root) : count(c), root(root) {}

  static u32 mask(u32 hash, u32 shift) {
    return (hash >> shift) & 0x01f;
  }
  static u32 bitpos(u32 hash, u32 shift) {
    return 1 << mask(hash, shift);
  }

  enum class Type { Leaf, Bitmap };

  struct Node {
    Type type;
    constexpr Node(Type t) : type(t) {}
  };

  struct LeafNode : public Node {
    // Hold an actual entry
    u32       hash;
    KeyType   key;
    ValueType value;

    LeafNode(u32 hash, KeyType key, ValueType value)
      : Node(Type::Leaf), hash(hash), key(key), value(value) {}
  };


  template <typename T>
  static void arraycopy(
      T* src, size_t srcpos,
      T* dst, size_t dstpos,
      size_t length)
  {
    // Copies an array from the specified source array, beginning at
    // the specified position, to the specified position of the destination
    // array. A subsequence of array components are copied from the source
    // array referenced by src to the destination array referenced by dest.
    // The number of components copied is equal to the length argument.
    // The components at positions srcPos through srcPos+length-1 in the
    // source array are copied into positions destPos through
    // destPos+length-1, respectively, of the destination array.
    std::memcpy((void*)(dst+dstpos), (const void*)(src+srcpos),
      sizeof(T)*length);
  }


  struct BitmapNode : public Node {
    // Holds other nodes
    u32   bitmap; // Which children this node has
    //u32   shift;  // This node's level (0, 5, 10, 15, 20, 25, 30)
    void** array; // keys and values as [k,v, k,v, ...]

    static BitmapNode* Empty;

    BitmapNode(u32 bitmap, void** array)
        : Node(Type::Bitmap), bitmap(bitmap), array(array) {}

    u32 index(u32 bit) {
      return popcount_u32(bitmap & (bit - 1));
    }

    BitmapNode* assoc(u32 shift, u32 hash, KeyType key, ValueType value) {
      u32 bit = bitpos(hash, shift);
      u32 idx = index(bit);

      if ((bitmap & bit) != 0) {
        // todo
      } else {
        u32 n = popcount_u32(bitmap);
        assert(n < 16); // todo

        ValueType* newArray = new Node*[2*(n+1)]; // [2-32]
        arraycopy<void*>(array, 0, newArray, 0, 2*idx);
        
        newArray[2*idx] = key;
        newArray[2*idx+1] = value;

        arraycopy(array, 2*idx, newArray, 2*(idx+1), 2*(n-idx));
        return new BitmapNode(bitmap | bit, newArray);

        // if (n == 0) {
        //   return new LeafNode(hash, key, value);
        // } else if (n->type == Type::Leaf) {
        //   // Turn into a BitmapNode with two leaf nodes, unless we are replacing
        //   // the key-value of the target leaf node.
        //   LeafNode* ln = new LeafNode(hash, key, value);
        //   if (((LeafNode*)n)->hash == hash /*&&ln->key==key*/ ) {
        //     // Edge-case: Replace the value of the existing leaf node
        //     return ln;
        //   }
        // }
        // return 0;
      }
    }

  };


  constexpr Map() : count(0), root(new BitmapNode(0,0)) {}


  static Map* assoc(Map* m, KeyType key, ValueType value) {
    u32 hash = KeyHash(key);

    if (m->root == 0) {
      LeafNode* ln = new LeafNode(hash, key, value);
      return new Map(1, ln);
    } else {
      return 0; // todo
    }
  }

  static ValueType get(Map* m, KeyType key) {
    if (m->root != 0) {
      u32 hash = KeyHash(key);
      if (m->root->type == Type::Leaf) {
        LeafNode* ln = (LeafNode*)m->root;
        if (ln->hash == hash /*&& ln->key == key*/) {
          return ln->value;
        }
      } else {

      }
    }
    return 0;
  }


  static Node* find(Node* n, u32 hash, void* key) {
    if (n->type == Type::Bitmap) {
      BitmapNode* bn = (BitmapNode*)n;
      u32 bit = bitpos(hash, bn->shift);
      if ((bn->bitmap & bit) != 0) {
        return find(bn->nodes[bn->index(bit)], hash, key);
      }
    } else if (n->type == Type::Leaf) {
      LeafNode* ln = (LeafNode*)n;
      if (ln->hash == hash /*&& ln->key == key*/) {
        return ln;
      }
    }
    return 0;
  }
};


} // namespace lum
#endif // _LUM_MAP_H_
