#pragma once

#include <cstddef>
#include <functional>
#include <vector>
#include <list>
#include <optional>

namespace dtf
{
    // a hash table implementation that maintains insertion order using std::list
    template<class K, class V>
    class Map
    {
    public:

        struct Item
        {
            K key;
            V value;

            Item(K &&key, V &&value) :
                    key(key),
                    value(value)
            {}

            Item(const K &key, const V &value) :
                    key(key),
                    value(value)
            {}

            Item(Item &&item) noexcept :
                    key(std::move(item.key)),
                    value(std::move(item.value))
            {}

            Item(const Item &item) noexcept :
                    key(item.key),
                    value(item.value)
            {}
        };

        using IterType = typename std::list<Item>::iterator;
        using Chain = std::list<IterType>;

        Map()
        {
            constexpr int n = 10;

            m_bucket = new Chain[n];
            m_capacity = n;

            for (int i = 0; i < n; i++)
                m_bucket[i] = Chain();
        }

        ~Map()
        {
            delete[] m_bucket;
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

        Item& set(const K &key, const V &value)
        {
            Item item(key, value);
            return set_item(item);
        }

        template<class ...A>
        Item& emplace(A &&...a)
        {
            Item item(a...);
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

            m_capacity = 10;
            m_bucket   = new Chain[m_capacity];
            m_size     = 0;

            for (int i = 0; i < m_capacity; i++)
                m_bucket[i] = Chain();
        }

        auto begin()
        {
            return m_items.begin();
        }

        auto end()
        {
            return m_items.end();
        }

    private:
        size_t m_size{};
        size_t m_capacity{};
        Chain *m_bucket{};
        std::list<Item> m_items;
        std::hash<K> m_hash;

        Item& set_item(Item &item)
        {
            m_size++;

            if (m_size >= m_capacity)
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
    };
}
