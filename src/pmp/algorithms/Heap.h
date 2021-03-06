//=============================================================================
// Copyright (C) 2011-2017 The pmp-library developers
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice, this
//   list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from
//   this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//=============================================================================
#pragma once
//=============================================================================

#include <vector>

//=============================================================================

template <class HeapEntry>
struct HeapInterfaceT
{
    //! Comparison of two HeapEntry's: strict less
    bool less(const HeapEntry& e1, const HeapEntry& e2);

    //! Comparison of two HeapEntry's: strict greater
    bool greater(const HeapEntry& e1, const HeapEntry& e2);

    //! Get the heap position of HeapEntry E
    int getHeapPosition(const HeapEntry& e);

    //! Set the heap position of HeapEntry E
    void setHeapPosition(HeapEntry& e, int i);
};

//=============================================================================

template <class HeapEntry, class HeapInterface>
class HeapT : private std::vector<HeapEntry>
{
public:
    typedef HeapT<HeapEntry, HeapInterface> This;

    //! Constructor
    HeapT() : HeapVector() {}

    //! Construct with a given \c HeapIterface.
    HeapT(const HeapInterface& Interface) : HeapVector(), m_interface(Interface)
    {
    }

    //! Destructor.
    ~HeapT(){};

    //! clear the heap
    void clear() { HeapVector::clear(); }

    //! is heap empty?
    bool empty() { return HeapVector::empty(); }

    //! returns the size of heap
    unsigned int size() { return (unsigned int)HeapVector::size(); }

    //! reserve space for N entries
    void reserve(unsigned int n) { HeapVector::reserve(n); }

    //! reset heap position to -1 (not in heap)
    void resetHeapPosition(HeapEntry h) { m_interface.setHeapPosition(h, -1); }

    //! is an entry in the heap?
    bool isStored(HeapEntry h) { return m_interface.getHeapPosition(h) != -1; }

    //! insert the entry h
    void insert(HeapEntry h)
    {
        This::push_back(h);
        upheap(size() - 1);
    }

    //! get the first entry
    HeapEntry front()
    {
        assert(!empty());
        return entry(0);
    }

    //! delete the first entry
    void popFront()
    {
        assert(!empty());
        m_interface.setHeapPosition(entry(0), -1);
        if (size() > 1)
        {
            entry(0, entry(size() - 1));
            HeapVector::resize(size() - 1);
            downheap(0);
        }
        else
            HeapVector::resize(size() - 1);
    }

    //! remove an entry
    void remove(HeapEntry h)
    {
        int pos = m_interface.getHeapPosition(h);
        m_interface.setHeapPosition(h, -1);

        assert(pos != -1);
        assert((unsigned int)pos < size());

        // last item ?
        if ((unsigned int)pos == size() - 1)
            HeapVector::resize(size() - 1);

        else
        {
            entry(pos, entry(size() - 1)); // move last elem to pos
            HeapVector::resize(size() - 1);
            downheap(pos);
            upheap(pos);
        }
    }

    //! update an entry: change the key and update the position to
    //! reestablish the heap property.
    void update(HeapEntry h)
    {
        int pos = m_interface.getHeapPosition(h);
        assert(pos != -1);
        assert((unsigned int)pos < size());
        downheap(pos);
        upheap(pos);
    }

    //! check heap condition
    bool check()
    {
        bool ok(true);
        unsigned int i, j;
        for (i = 0; i < size(); ++i)
        {
            if (((j = left(i)) < size()) &&
                m_interface.greater(entry(i), entry(j)))
            {
                std::cerr << "Heap condition violated\n";
                ok = false;
            }
            if (((j = right(i)) < size()) &&
                m_interface.greater(entry(i), entry(j)))
            {
                std::cerr << "Heap condition violated\n";
                ok = false;
            }
        }
        return ok;
    }

private:
    // typedef
    typedef std::vector<HeapEntry> HeapVector;

    //! Upheap. Establish heap property.
    void upheap(unsigned int idx)
    {
        HeapEntry h = entry(idx);
        unsigned int parentIdx;

        while ((idx > 0) && m_interface.less(h, entry(parentIdx = parent(idx))))
        {
            entry(idx, entry(parentIdx));
            idx = parentIdx;
        }

        entry(idx, h);
    }

    //! Downheap. Establish heap property.
    void downheap(unsigned int idx)
    {
        HeapEntry h = entry(idx);
        unsigned int childIdx;
        unsigned int s = size();

        while (idx < s)
        {
            childIdx = left(idx);
            if (childIdx >= s)
                break;

            if ((childIdx + 1 < s) &&
                (m_interface.less(entry(childIdx + 1), entry(childIdx))))
                ++childIdx;

            if (m_interface.less(h, entry(childIdx)))
                break;

            entry(idx, entry(childIdx));
            idx = childIdx;
        }

        entry(idx, h);
    }

    //! Get the entry at index idx
    inline HeapEntry entry(unsigned int idx)
    {
        assert(idx < size());
        return (This::operator[](idx));
    }

    //! Set entry H to index idx and update H's heap position.
    inline void entry(unsigned int idx, HeapEntry h)
    {
        assert(idx < size());
        This::operator[](idx) = h;
        m_interface.setHeapPosition(h, idx);
    }

    //! Get parent's index
    inline unsigned int parent(unsigned int i) { return (i - 1) >> 1; }

    //! Get left child's index
    inline unsigned int left(unsigned int i) { return (i << 1) + 1; }

    //! Get right child's index
    inline unsigned int right(unsigned int i) { return (i << 1) + 2; }

    //! Instance of HeapInterface
    HeapInterface m_interface;
};

//=============================================================================
