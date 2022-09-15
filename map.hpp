#pragma once

#include <cstddef>
#include <functional>
#include <vector>
#include <list>
#include <optional>
#include <initializer_list>

#include <iostream>

namespace dtf
{
    template<typename K, typename V>
    struct Record
    {
        K key;
        V value;

        Record(K &&key, V &&value) :
                key(std::move(key)),
                value(std::move(value))
        {}

        Record(const K &key, const V &value) :
                key(key),
                value(value)
        {}

        Record(Record &&item) noexcept :
                key(std::move(item.key)),
                value(std::move(item.value))
        {}

        Record(const Record &item) noexcept :
                key(item.key),
                value(item.value)
        {}

        Record& operator=(const Record &item)
        {
            key = item.key;
            value = item.value;
            return *this;
        }

        Record& operator=(Record &&item) noexcept
        {
            key = std::move(item.key);
            value = std::move(item.value);
            return *this;
        }
    };

    // a hash table implementation that maintains insertion order using std::list
    template<class K, class V>
    class Map
    {
    public:
        using IterType = typename std::list<Record<K, V>>::iterator;
        using Chain = std::list<IterType>;

        Map(std::initializer_list<Record<K&&, V&&>> list)
        {
            m_size = list.size();

            construct(m_size * 2);

            for (auto &[key, value] : list)
                set(std::forward<K>(key), std::forward<V>(value));
        }

        Map()
        {
            construct(10);
        }

        Map(Map<K, V> &&map) noexcept
        {
           move(std::forward<Map<K, V>>(map));
        }

        Map(const Map<K, V> &map)
        {
            copy(map);
        }

        ~Map()
        {
            delete[] m_bucket;
        }

        Map<K, V>& operator=(Map<K, V> &&map) noexcept
        {
            move(std::forward<Map<K, V>>(map));
            return *this;
        }

        Map<K, V>& operator=(const Map<K, V> &map)
        {
            copy(map);
            return *this;
        }

        [[nodiscard]]
        constexpr inline
        size_t size() const
        {
            return m_size;
        }

        [[nodiscard]]
        constexpr inline
        size_t capacity() const
        {
            return m_capacity;
        }

        [[nodiscard]]
        constexpr inline
        bool empty() const
        {
            return !m_size;
        }

        Record<K, V>& set(K &&key, V &&value)
        {
            Record<K, V> item(std::forward<K>(key), std::forward<V>(value));
            return set_item(item);
        }

        Record<K, V>& set(const K &key, V &&value)
        {
            Record<K, V> item(key, value);
            return set_item(item);
        }

        template<class ...A>
        Record<K, V>& emplace(A &&...a)
        {
            Record<K, V> item(a...);
            return set_item(item);
        }

        // returns a pointer instead of an optional because realistically it would end in the same operation ie checking if its valid and using it
        // this just results in less verbose code
        V* get(const K &key) const
        {
            return search(key);
        }

        V& operator[](const K &key)
        {
            V *value = search(key);

            if (!value)
                return set(key, V()).value;

            return *value;
        }

        V& get(const K &key, const V &def_value) const
        {
            V *value = search(key);
            return value ? *value : const_cast<V&>(def_value);
        }

        // gets all values of duplicate keys
        std::vector<V*> get_all(const K &key) const
        {
            size_t h = hash(key);

            std::vector<V*> output;

            std::cout << m_bucket[h].size() << '\n';

            for (IterType item : m_bucket[h])
            {
                if (item->key == key)
                    output.push_back(&item->value);
            }

            return output;
        }

        bool contains(const K &key) const
        {
            return search(key);
        }

        // returns true if the entry was erased
        bool erase(const K &key)
        {
            size_t h = hash(key);
            Chain &chain = m_bucket[h];

            for (auto it = chain.begin(); it != chain.end(); it++)
            {
                if ((*it)->key == key)
                {
                    m_items.erase(*it);
                    chain.erase(it);
                    m_size--;
                    return true;
                }
            }
            return false;
        }

        // removes all entries in map
        void clear()
        {
            m_items.clear();

            delete[] m_bucket;

            m_size = 0;
            construct(10);
        }

        auto begin() const
        {
            return m_items.begin();
        }

        auto end() const
        {
            return m_items.end();
        }

    private:
        size_t m_size{};
        size_t m_capacity{};
        Chain *m_bucket{};
        std::list<Record<K, V>> m_items;
        std::hash<K> m_hash;

        Record<K, V>& set_item(Record<K, V> &item)
        {
            if (++m_size >= m_capacity)
                rehash();

            size_t h = hash(item.key);

            Chain &chain = m_bucket[h];

            IterType iter = m_items.emplace(m_items.end(), std::move(item));
            chain.emplace_back(iter);

            return *iter;
        }

        constexpr inline
        size_t hash(const K &k) const
        {
            return m_hash(k) % m_capacity;
        }

        V* search(const K &key) const
        {
            size_t h = hash(key);

            for (IterType item : m_bucket[h])
            {
                if (item->key == key)
                    return &item->value;
            }
            return nullptr;
        }

        inline void construct(int n)
        {
            m_capacity = n;
            m_bucket   = new Chain[m_capacity];

            for (int i = 0; i < m_capacity; i++)
                m_bucket[i] = Chain();
        }

        void rehash()
        {
            const size_t n = m_capacity;
            m_capacity *= 2;

            auto temp = new Chain[m_capacity];

            size_t i;

            for (i = 0; i < n; i++)
            {
                temp[i] = Chain();
                Chain &old = m_bucket[i];

                if (old.empty())
                    continue;

                for (IterType item : old)
                {
                    size_t h = hash(item->key);
                    Chain *chain = &temp[h];

                    if (!chain)
                        temp[h] = Chain();

                    chain->push_back(item);
                }
            }

            for(; i < m_capacity; i++)
                temp[i] = Chain();

            delete[] m_bucket;

            m_bucket = temp;
        }

        void move(Map<K, V> &&map) noexcept
        {
            m_bucket = map.m_bucket;
            map.m_bucket = nullptr;

            m_size = map.m_size;
            m_capacity = map.m_capacity;

            m_items = std::move(map.m_items);
        }

        void copy(const Map<K, V> &map)
        {
            m_items = map.m_items;

            m_capacity = map.m_capacity;
            m_size = map.m_size;

            m_bucket = new Chain[m_capacity];

            for (size_t i = 0; i < m_capacity; i++)
                m_bucket[i] = map.m_bucket[i];
        }
    };
}
