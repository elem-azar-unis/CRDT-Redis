//
// Created by user on 18-11-15.
//
#include "queueLog.h"


void queueLog::shift_up(int s)
{
    int j = s, i = (j - 1) / 2;
    auto temp = heap[j];
    while (j > 0)
    {
        if (*heap[i] >= *temp)break;
        else
        {
            heap[j] = heap[i];
            heap[j]->index = j;
            j = i;
            i = (i - 1) / 2;
        }
    }
    temp->index = j;
    heap[j] = temp;
}

void queueLog::shift_down(int s)
{
    int i = s, j = 2 * i + 1, tail = static_cast<int>(heap.size() - 1);
    auto temp = heap[i];
    while (j <= tail)
    {
        if (j < tail && *heap[j] <= *heap[j + 1])j++;
        if (*temp >= *heap[j])break;
        else
        {
            heap[i] = heap[j];
            heap[i]->index = i;
            i = j;
            j = i * 2 + 1;
        }
    }
    temp->index = i;
    heap[i] = temp;
}

void queueLog::add(int k, double v)
{
    mtx.lock();
    if (map.find(k) == map.end())
    {
        auto e = new element(k, v);
        map[k] = e;
        heap.push_back(e);
        shift_up(static_cast<int>(heap.size() - 1));
    }
    mtx.unlock();
}

void queueLog::inc(int k, double i)
{
    if (i == 0)return;
    mtx.lock();
    if (map.find(k) != map.end())
    {
        auto e = map[k];
        e->value += i;
        if (i > 0)
            shift_up(e->index);
        else
            shift_down(e->index);
    }
    mtx.unlock();
}

void queueLog::rem(int k)
{
    mtx.lock();
    if (map.find(k) != map.end())
    {
        auto e = map[k];
        map.erase(map.find(k));
        heap[e->index] = heap.back();
        heap.erase(heap.end() - 1);
        shift_down(e->index);
        delete e;
    }
    mtx.unlock();
}

void queueLog::max(int k, double v)
{
    int ak;
    double av;

    mtx.lock();
    if (!heap.empty())
    {
        ak = heap[0]->name;
        av = heap[0]->value;
    }
    else
    {
        ak = -1;
        av = -1;
    }
    mtx.unlock();

    max_mtx.lock();
    max_log.emplace_back(k, v, ak, av);
    max_mtx.unlock();
}
