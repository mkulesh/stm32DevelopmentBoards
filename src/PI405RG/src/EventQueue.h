/*******************************************************************************
 * Test unit for the development board: STM32F405RGT6
 * *****************************************************************************
 * Copyright (C) 2016-2017 Mikhail Kulesh
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef EVENT_QUEUE_H_
#define EVENT_QUEUE_H_

#include <array>

template <typename T, std::size_t N> class EventQueue
{
private:

    std::array<T, N + 1> buffer;
    size_t head, tail;

public:

    explicit EventQueue() : head{0}, tail{0}
    {
        //empty
    }

    void put (T item)
    {
        if (head < buffer.size())
        {
            buffer[head] = item;
        }
        head = (head + 1) % N;
        if(head == tail)
        {
            tail = (tail + 1) % N;
        }
    }

    T get (void)
    {
        auto val = buffer[tail];
        tail = (tail + 1) % N;
        return val;
    }

    void reset (void)
    {
        head = tail;
    }

    bool empty (void) const
    {
        return head == tail;
    }

    bool full (void) const
    {
        return ((head + 1) % N) == tail;
    }

    size_t getHead () const
    {
        return head;
    }
    size_t getTail () const
    {
        return tail;
    }
};

#endif
