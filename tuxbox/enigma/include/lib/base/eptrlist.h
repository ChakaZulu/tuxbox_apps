#ifndef _E_PTRLIST_
#define _E_PTRLIST_

#include <list>

template <class T>
class ePtrList : public std::list<T*>
{
	bool autoDelete;
	iterator cur;
public:
	ePtrList(): autoDelete(false)	{	}
	~ePtrList();
	class iterator;
	class const_iterator;
	class reverse_iterator;
	class const_reverse_iterator;
	iterator begin()	{		return std::list<T*>::begin();	}
	iterator end()	{		return std::list<T*>::end();	}
	const_iterator begin() const	{		return std::list<T*>::begin();	}
	const_iterator end() const	{		return std::list<T*>::end();	}
	reverse_iterator rbegin()	{		return std::list<T*>::rbegin();	}
	reverse_iterator rend()	{		return std::list<T*>::rend();	}
	const_reverse_iterator rbegin() const	{		return std::list<T*>::rbegin();	}
	const_reverse_iterator rend() const	{		return std::list<T*>::rend();	}
// overwritted sort method
	void sort()	{		std::list<T*>::sort(ePtrList<T>::less()); }	
// changed methods for autodelete and current implementation
	void remove(T* t);
	iterator erase(iterator& it);
	iterator erase(iterator from, iterator to);
	void clear()	{		erase(std::list<T*>::begin(),std::list<T*>::end());	}
	void pop_back();
	void pop_front();
// added methods for current implementation
	T* take();
	T* current()	{		return *cur;	}
	T* next()	{		return *++cur;	}
	T* prev()	{		return *--cur;	}
	T* first()	{		return *(cur = begin());	}
	T* last()	{		return *(cur = --end());	}
	const T* current() const	{		return *cur;	}
	const T* next() const	{		return *++cur;	}
	const T* prev()	const {		return *--cur;	}
	const	T* first() const	{		return *(cur = begin());	}
	const	T* last()	const {		return *(cur = --end());	}
// added operator methods
	operator bool()	{		return !empty();	}
	bool operator!()	{		return empty();	}
	operator iterator()	{		return begin();	}
	operator const_iterator() const	{		return begin();	}
	operator reverse_iterator()	{		return rbegin();	}
	operator const_reverse_iterator() const	{		return rbegin();	}
// added methods for autodelete implementation
	void setAutoDelete(bool b)	{		autoDelete=b;	}
	bool isAutoDelete()	{		return autoDelete;	}
	struct less
	{
		bool operator() (const T* t1, const T* t2)
		{
			return (*t1 < *t2);
		}
	};
};

/////////////////// iterator class /////////////////////////////
template <class T>
class ePtrList<T>::iterator : public std::list<T*>::iterator
{
public:
	// Constructors
	iterator(const std::list<T*>::iterator& Q)		{			_M_node = Q._M_node;		}
//	iterator& operator=(const std::list<T*>::iterator & Q)			{		return _M_node = Q._M_node;	}
	// changed operator for pointer
	T* operator->() const { return ((_Node*) _M_node)->_M_data; }
	operator T&() const { return (*((_Node*) _M_node)->_M_data); }
};

/////////////////// const_iterator class /////////////////////////////
template <class T>
class ePtrList<T>::const_iterator : public std::list<T*>::const_iterator
{
public:
	// Constructors
	const_iterator(const std::list<T*>::const_iterator& Q)		{			_M_node = Q._M_node;		}
//	const_iterator& operator=(const std::list<T*>::const_iterator & Q)			{		return _M_node = Q._M_node;	}
	// changed operator for pointer
	T* operator->() const { return ((_Node*) _M_node)->_M_data; }
	operator T&() const { return (*((_Node*) _M_node)->_M_data); }
};

/////////////////// reverse_iterator class /////////////////////////////
template <class T>
class ePtrList<T>::reverse_iterator : public std::list<T*>::reverse_iterator
{
public:
	// Constructors
	reverse_iterator(const std::list<T*>::reverse_iterator& Q)		{			_M_node = Q._M_node;		}
//	reverse_iterator& operator=(const std::list<T*>::reverse_iterator & Q)			{		return _M_node = Q._M_node;	}
	// changed operator for pointer
	T* operator->() const { return ((_Node*) _M_node)->_M_data; }
	operator T&() const { return (*((_Node*) _M_node)->_M_data); }
};

/////////////////// const_reverse_iterator class /////////////////////////////
template <class T>
class ePtrList<T>::const_reverse_iterator : public std::list<T*>::const_reverse_iterator
{
public:
	// Constructors
	const_reverse_iterator(const std::list<T*>::const_reverse_iterator& Q)		{			_M_node = Q._M_node;		}
//	const_reverse_iterator& operator=(const std::list<T*>::const_reverse_iterator & Q)			{		return _M_node = Q._M_node;	}
	// changed operator for pointer
	T* operator->() const { return ((_Node*) _M_node)->_M_data; }
	operator T&() const { return (*((_Node*) _M_node)->_M_data); }
};

/////////////////// ePtrList destructor /////////////////////////////
template <class T>
inline ePtrList<T>::~ePtrList()
{
	if (autoDelete)
	{
		for (std::list<T*>::iterator it(std::list<T*>::begin()); it != std::list<T*>::end(); it++)
			delete *it;
	}
}

/////////////////// ePtrList remove(T*) /////////////////////////
template <class T>
inline void ePtrList<T>::remove(T* t)
{
	if (autoDelete)
		delete t;
	
	while (*cur == t && cur != end() )
		cur++;

	std::list<T*>::remove(t);
}

/////////////////// ePtrList erase(iterator) ///////////////////////
template <class T>
inline ePtrList<T>::iterator ePtrList<T>::erase(iterator& it)
{
	if (autoDelete)
		delete *it;
	
	iterator tmp = std::list<T*>::erase(it);
	
	if (*cur == *it)
		cur = tmp;
	
  return tmp;
}

/////////////////// ePtrList erase(iterator from. iterator to) //////////////////
template <class T>
inline ePtrList<T>::iterator ePtrList<T>::erase(iterator from, iterator to)
{
	while (from != to)
		from = erase(from);

	return from;
}

/////////////////// ePtrList pop_back() ////////////////////
template <class T>
inline void ePtrList<T>::pop_back()
{
	if (autoDelete && *std::list<T*>::end())
		delete *std::list<T*>::end();

	if (*cur == *end())
		cur++;

	std::list<T*>::pop_back();
}

/////////////////// ePtrList pop_front() ////////////////////
template <class T>
inline void ePtrList<T>::pop_front()
{
	if (autoDelete && *std::list<T*>::begin())
		delete *std::list<T*>::begin();

	if (*cur == *begin())
		cur++;

	std::list<T*>::pop_front();
}

/////////////////// ePtrList take() ////////////////////
template <class T>
inline T* ePtrList<T>::take()
{
	T* tmp = *cur;

	cur = std::list<T*>::erase(cur);

	return tmp;
}

#endif // _E_PTRLIST
