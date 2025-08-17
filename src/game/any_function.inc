namespace boost
{
    class any
    {
    public: // structors

        any()
          : content(0)
        {
        }

        template<typename ValueType>
        any(const ValueType & value)
          : content(new holder<ValueType>(value))
        {
        }

        any(const any & other)
          : content(other.content ? other.content->clone() : 0)
        {
        }

        ~any()
        {
            delete content;
        }

    public: // modifiers

        any & swap(any & rhs)
        {
            std::swap(content, rhs.content);
            return *this;
        }

        template<typename ValueType>
        any & operator=(const ValueType & rhs)
        {
            any(rhs).swap(*this);
            return *this;
        }

        any & operator=(const any & rhs)
        {
            any(rhs).swap(*this);
            return *this;
        }

	void operator ()(func_arg_type func_arg)
	{
	    (*content)(func_arg);
	}

    public: // queries

        bool empty() const
        {
            return !content;
        }

    private: // types

        class placeholder
        {
        public: // structors
    
            virtual ~placeholder()
            {
            }

        public: // queries

            virtual placeholder * clone() const = 0;

	    virtual void operator()(func_arg_type func_arg) = 0;
    
        };

        template<typename ValueType>
        class holder : public placeholder
        {
        public: // structors

            holder(const ValueType & value)
              : held(value)
            {
            }

        public: // queries

            virtual placeholder * clone() const
            {
                return new holder(held);
            }

	    virtual void operator ()(func_arg_type func_arg)
	    {
		held(func_arg);
	    }

        public: // representation

            ValueType held;

        };

    private: // representation

        template<typename ValueType>
        friend ValueType * any_cast(any *);

        placeholder * content;

    };

}
