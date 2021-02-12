namespace rf
{
    template<typename T, int I = 0>
    class VList
    {
        T* head;
        int num_elements;

    public:
        class Iterator
        {
            T* current_;

        public:
            Iterator(T* ptr) : current_(ptr) {}

            T& operator*() const
            {
                return *current_;
            }

            Iterator& operator++()
            {
                current_ = current_->next[I];
                return *this;
            }

            Iterator operator++(int)
            {
                Iterator copy = *this;
                current_ = current_->next[I];
                return copy;
            }

            bool operator==(Iterator& other) const
            {
                return current_ == other.current_;
            }

            bool operator!=(Iterator& other) const
            {
                return current_ != other.current_;
            }
        };

        class ConstIterator
        {
            const T* current_;

        public:
            ConstIterator(const T* ptr) : current_(ptr) {}

            const T& operator*() const
            {
                return *current_;
            }

            ConstIterator& operator++()
            {
                current_ = current_->next[I];
                return *this;
            }

            ConstIterator operator++(int)
            {
                ConstIterator copy = *this;
                current_ = current_->next[I];
                return copy;
            }

            bool operator==(ConstIterator& other) const
            {
                return current_ == other.current_;
            }

            bool operator!=(ConstIterator& other) const
            {
                return current_ != other.current_;
            }
        };

        T* first()
        {
            return head;
        }

        const T* first() const
        {
            return head;
        }

        T* next(T* elem)
        {
            return elem->next[I];
        }

        const T* next(const T* elem) const
        {
            return elem->next[I];
        }

        int size() const
        {
            return num_elements;
        }

        bool empty() const
        {
            return num_elements == 0;
        }

        Iterator begin()
        {
            return Iterator{head};
        }

        Iterator begin() const
        {
            return ConstIterator{head};
        }

        Iterator end()
        {
            return Iterator{nullptr};
        }

        Iterator end() const
        {
            return ConstIterator{nullptr};
        }
    };
}
