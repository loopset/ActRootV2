#ifndef ActInterval_h
#define ActInterval_h

#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <map>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

namespace ActCluster
{
    template <typename T>
    class Interval
    {
    private:
        T fStart;
        T fStop;

    public:
        Interval() = default;
        Interval(const T& start, const T& stop) : fStart(start), fStop(stop) {}

        // Basic overloads of operators
        friend bool operator==(const Interval& one, const Interval& other)
        {
            return one.GetStart() == other.GetStart() && one.GetStop() == other.GetStop();
        }
        friend bool operator<(const Interval& one, const Interval& other)
        {
            return one.GetStart() < other.GetStart() ||
                   one.GetStart() == other.GetStart() && one.GetStop() < other.GetStop();
        }
        friend bool operator>(const Interval& one, const Interval& other) { return operator<(other, one); }
        friend bool operator<=(const Interval& one, const Interval& other) { return !(operator>(one, other)); }
        friend bool operator>=(const Interval& one, const Interval& other) { return !(operator<(one, other)); }

        // Getters
        // Get start of interval
        T GetStart() const { return fStart; }
        // Get end of interval
        T GetStop() const { return fStop; }

        // Utility functions
        // Get overlap with intervals using <= operator
        bool Overlaps(const Interval& other) const { return fStart <= other.GetStop() && other.GetStart() <= fStop; }
        // Contains values
        bool Contains(const T& point) const { return fStart <= point && point <= fStop; }
        // Intersect with another interval
        Interval Intersect(const Interval& other) const
        {
            if(!Overlaps(other))
                return {};
            return Interval {std::max(fStart, other.GetStart()), std::min(fStop, other.GetStop())};
        }
        // Get size of interval!
        T Length() { return (fStop + 1) - fStart; } //+1 since this is a []-type interval, not usual [)
        // Distance to other interval
        T Distance(const Interval& other) //-1 by the same means
        {
            // Check overlap
            if(Overlaps(other))
                return {};
            // Sort them
            if(*this < other)
            {
                return other.GetStart() - fStop - 1;
            }
            else
            {
                return fStart - other.GetStop() - 1;
            }
        }
        // Output operator
        friend std::ostream& operator<<(std::ostream& out, const Interval<T>& interval)
        {
            out << "[" << interval.GetStart() << ", " << interval.GetStop() << "]";
            return out;
        }
    };

    template <typename T>
    class IntervalMap
    {
    private:
        std::map<T, std::vector<Interval<T>>> fMap;

    public:
        void Add(T key, const Interval<T>& interval) { fMap[key].push_back(interval); }
        std::vector<Interval<T>> GetIntervals(T key) { return fMap[key]; }
        const std::map<T, std::vector<Interval<T>>> GetMap() const { return fMap; }
        bool IsSplit(T key) { return fMap[key].size() > 1; }
        size_t GetSizeInKey(T key) { return fMap[key].size(); }
        void BuildFromSet(T key, const std::set<T>& set)
        {
            T start {*set.begin()};
            T stop {start};
            auto it {set.begin()};
            auto old {it};
            if(std::next(it) == set.end())
                fMap[key].push_back({start, stop});
            else
            {
                for(it = std::next(it); it != set.end(); it++)
                {
                    // std::cout<<"Old = "<<*old<<'\n';
                    // std::cout<<"It  = "<<*it<<'\n';
                    // std::cout<<"Diff = "<<std::abs(*it - *old)<<'\n';
                    if(std::abs(*it - *old) <= 1)
                        stop = *it;
                    else
                    {
                        fMap[key].push_back({start, stop});
                        // Redefine start
                        start = *it;
                        stop = start;
                    }
                    if(std::next(it) == set.end())
                    {
                        fMap[key].push_back({start, stop});
                    }
                    old = it;
                }
            }
            // Sort vector (lower intervals at beginning)
            std::sort(fMap[key].begin(), fMap[key].end());
        }
        T GetMaximumSeparation(T key)
        {
            T max {};
            for(auto out = fMap[key].begin(); out != fMap[key].end(); out++)
            {
                for(auto in = out + 1; in != fMap[key].end(); in++)
                {
                    auto dist {out->Distance(*in)};
                    if(dist > max)
                        max = dist;
                }
            }
            return max;
        }
        T GetMaximumSize(T key)
        {
            T max {};
            for(auto it = fMap[key].begin(); it != fMap[key].end(); it++)
            {
                auto length {it->Length()};
                if(length > max)
                    max = length;
            }
            return max;
        }
        std::pair<T, T> GetKeyAndMaxSep()
        {
            T axis {};
            T max {};
            for(const auto& [key, vec] : fMap)
            {
                auto dist {GetMaximumSeparation(key)};
                if(dist > max)
                {
                    axis = key;
                    max = dist;
                }
            }
            return {axis, max};
        }
        T GetKeyAtSep(T threshold, int depth = 2)
        {
            T axis {};
            int idx {};
            bool isContiguous {false};
            for(const auto& [key, vec] : fMap)
            {
                auto dist {GetMaximumSeparation(key)};
                if(dist > threshold)
                {
                    if(!isContiguous)
                        idx = 0;
                    if(idx == 0)
                        axis = key;
                    if(idx == depth - 1) // break after depth is achieved
                        break;
                    idx++;
                    isContiguous = true;
                }
                else
                    isContiguous = false;
            }
            return axis;
        }
        T GetKeyAtLength(T threshold, int depth = 2)
        {
            T axis {};
            int idx {};
            bool isContiguous {false};
            for(const auto& [key, vec] : fMap)
            {
                auto length {GetMaximumSize(key)};
                if(length > threshold || IsSplit(key))
                {
                    if(!isContiguous)
                        idx = 0;
                    if(idx == 0)
                        axis = key;
                    if(idx == depth - 1)
                        break;
                    idx++;
                    isContiguous = true;
                }
                else
                    isContiguous = false;
            }
            return axis;
        }
        void Print() const
        {
            for(const auto& [key, vec] : fMap)
            {
                std::cout << "-> Key : " << key << '\n';
                for(const auto& val : vec)
                    std::cout << "  " << val << '\n';
            }
        }
    };

} // namespace ActCluster
#endif // !ActInterval_h
