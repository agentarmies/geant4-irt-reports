#pragma once
#include "G4TrackList.hh"
#include <stdexcept>

// Same public sublist order as G4ManyFastLists, without its invalid end sentinel.
// The visitor may append associated sublists, including occupied ones: this
// live forward walk reaches them in the same pass. It must not remove/reorder
// associated sublists or insert/remove/reorder nodes within an existing sublist.
template<class Action>
void forEachIRTTrack(G4TrackManyList& list, Action action) {
  if(list.size()==0)return;
  auto* lists=list.begin().fLists;
  for(auto outer=lists->begin();outer!=lists->end();++outer) {
    auto* tracks=*outer;
    if(!tracks)throw std::runtime_error("null IRT chemistry sublist");
    for(auto it=tracks->begin();it!=tracks->end();++it)action(*it);
  }
}
