#ifndef STONE_PAGE_H
#define STONE_PAGE_H

namespace stone {
  // these are in common/page.cc
  extern unsigned _page_size;
  extern unsigned long _page_mask;
  extern unsigned _page_shift;
}

#endif


#define STONE_PAGE_SIZE stone::_page_size
#define STONE_PAGE_MASK stone::_page_mask
#define STONE_PAGE_SHIFT stone::_page_shift


