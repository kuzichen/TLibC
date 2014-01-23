#include "tlibc/platform/tlibc_platform.h"
#include "tlibc/core/tlibc_timer.h"
#include "tlibc/core/tlibc_list.h"
#include "tlibc/core/tlibc_error_code.h"

#include <assert.h>

void tlibc_timer_init(tlibc_timer_t *self, tuint64 jiffies)
{
	int i;

	self->jiffies = jiffies;

	for(i = 0;i < TLIBC_TVR_SIZE; ++i)
	{
		tlibc_list_init(&self->tv1.vec[i]);
	}

	for(i = 0;i < TLIBC_TVN_SIZE; ++i)
	{
		tlibc_list_init(&self->tv2.vec[i]);
	}

	for(i = 0;i < TLIBC_TVN_SIZE; ++i)
	{
		tlibc_list_init(&self->tv3.vec[i]);
	}

	for(i = 0;i < TLIBC_TVN_SIZE; ++i)
	{
		tlibc_list_init(&self->tv4.vec[i]);
	}

	for(i = 0;i < TLIBC_TVN_SIZE; ++i)
	{
		tlibc_list_init(&self->tv5.vec[i]);
	}
}


void tlibc_timer_pop(tlibc_timer_entry_t *self)
{
	tlibc_list_del(&self->entry);
}


void tlibc_timer_push(tlibc_timer_t *self, tlibc_timer_entry_t *timer)
{
	tuint64 expires = timer->expires;
	tuint64 idx;
	TLIBC_LIST_HEAD *vec;
	int i;
	
	if(expires <= self->jiffies)
	{
		vec = self->tv1.vec + (self->jiffies & TLIBC_TVR_MASK);
	}
	else
	{
		idx = expires - self->jiffies;
		if(idx < TLIBC_TVR_SIZE)
		{
			i = expires & TLIBC_TVR_MASK;
			vec = self->tv1.vec + i;
		}		
		else if(idx < 1 << (TLIBC_TVR_BITS + TLIBC_TVN_BITS))
		{
			i = (expires >> TLIBC_TVR_BITS) & TLIBC_TVN_MASK;
			vec = self->tv2.vec + i;
		}
		else if(idx < 1 << (TLIBC_TVR_BITS + 2 * TLIBC_TVN_BITS))
		{
			i = (expires >> (TLIBC_TVR_BITS + TLIBC_TVN_BITS)) & TLIBC_TVN_MASK;
			vec = self->tv3.vec + i;
		}
		else if(idx < 1 << (TLIBC_TVR_BITS + 3 * TLIBC_TVN_BITS))
		{
			i = (expires >> (TLIBC_TVR_BITS + 2 * TLIBC_TVN_BITS)) & TLIBC_TVN_MASK;
			vec = self->tv4.vec + i;
		}
		else
		{
			if(idx > TLIBC_MAX_TVAL)
			{
				idx = TLIBC_MAX_TVAL;
				expires = idx + self->jiffies;
			}
			i = (expires >> (TLIBC_TVR_BITS + 3 * TLIBC_TVN_BITS)) & TLIBC_TVN_MASK;
			vec = self->tv5.vec + i;
		}
	}
	tlibc_list_init(&timer->entry);
	tlibc_list_add(&timer->entry, vec);
}

#define INDEX(N) ((self->jiffies >> (TLIBC_TVR_BITS + (N) * TLIBC_TVN_BITS)) & TLIBC_TVN_MASK)

static int cascade(tlibc_timer_t *self, tlibc_timer_vec_t *tv, int index)
{
	tlibc_timer_entry_t *timer;
	TLIBC_LIST_HEAD tv_list;
	TLIBC_LIST_HEAD *tv_old;
	TLIBC_LIST_HEAD *iter;

	tv_old = tv->vec + index;

	tv_list.next= tv_old->next;	
	tv_list.next->prev = &tv_list;
	tv_list.prev = tv_old->prev;
	tv_list.prev->next = &tv_list;
	
	tlibc_list_init(tv_old);

	for(iter = tv_list.next; iter != &tv_list; )
	{
		TLIBC_LIST_HEAD *next = iter->next;
		timer = TLIBC_CONTAINER_OF(iter, tlibc_timer_entry_t, entry);
		//����ҪС�ĳ���Ŷ~~
		tlibc_timer_push(self, timer);
		iter = next;
	}

	return index;
}

TLIBC_ERROR_CODE tlibc_timer_tick(tlibc_timer_t *self, tuint64 jiffies)
{
	TLIBC_ERROR_CODE ret = E_TLIBC_AGAIN;

	if(self->jiffies <= jiffies)
	{
		int index = self->jiffies & TLIBC_TVR_MASK;
		TLIBC_LIST_HEAD *tv_old;

		if (!index &&
			(!cascade(self, &self->tv2, INDEX(0))) &&
			(!cascade(self, &self->tv3, INDEX(1))) &&
			!cascade(self, &self->tv4, INDEX(2)))
		{
			cascade(self, &self->tv5, INDEX(3));
		}
		++self->jiffies;

		tv_old = self->tv1.vec + index;
		if(!tlibc_list_empty(tv_old))
		{
			ret = E_TLIBC_NOERROR;
		}
		while(!tlibc_list_empty(tv_old))
		{
			tlibc_timer_entry_t *timer = TLIBC_CONTAINER_OF(tv_old->next, tlibc_timer_entry_t, entry);			
			tlibc_list_del(tv_old->next);
			timer->callback(timer);
		}
 	}

	return ret;
}