#include "core/tlibc_hash.h"
#include "core/tlibc_list.h"
#include "core/tlibc_error_code.h"

#include <memory.h>
#include <assert.h>

TLIBC_ERROR_CODE tlibc_hash_init(tlibc_hash_t *self, tlibc_hash_bucket_t *buckets, tuint32 size)
{
	tuint32 i;

	self->buckets = buckets;
	self->size = size;
	for(i = 0; i < self->size; ++i)
	{
		init_tlibc_list_head(&self->buckets[i].data_list);
		init_tlibc_list_head(&self->buckets[i].used_bucket_list);
		self->buckets[i].data_list_num = 0;
	}

	init_tlibc_list_head(&self->used_bucket_list);

	return E_TLIBC_NOERROR;
}

tuint32 tlibc_hash_key(const char* key, tuint32 key_size)
{
	tuint32 i, key_hash;
	key_hash = 0;
	for(i = 0; i < key_size; ++i)
	{
		key_hash = key_hash * 31 + key[i];
	}
	return key_hash;
}

void tlibc_hash_insert(tlibc_hash_t *self, const char* key, tuint32 key_size, tlibc_hash_head_t *val_head)
{
	tuint32 key_hash = tlibc_hash_key(key, key_size);
	tuint32 key_index = key_hash % self->size;
	tlibc_hash_bucket_t *bucket = &self->buckets[key_index];

	val_head->key = key;
	val_head->key_size = key_size;
	val_head->key_index = key_index;

	init_tlibc_list_head(&val_head->data_list);
	
	tlibc_list_add(&val_head->data_list, &bucket->data_list);
	if(bucket->data_list_num == 0)
	{
		tlibc_list_add(&bucket->used_bucket_list, &self->used_bucket_list);
	}	
	++bucket->data_list_num;
}

const tlibc_hash_head_t* tlibc_hash_find_const(const tlibc_hash_t *self, const char *key, tuint32 key_size)
{
	tuint32 key_hash = tlibc_hash_key(key, key_size);
	tuint32 key_index = key_hash % self->size;
	const tlibc_hash_bucket_t *bucket = &self->buckets[key_index];
	TLIBC_LIST_HEAD *iter;
	tuint32 i;
	for(iter = bucket->data_list.next, i = 0; iter != &bucket->data_list; iter = iter->next, ++i)
	{
		tlibc_hash_head_t *ele = TLIBC_CONTAINER_OF(iter, tlibc_hash_head_t, data_list);
		if(i >= bucket->data_list_num)
		{
			//���ִ�е���� ˵��������˵�����صĴ��� �����ڴ�Խ�絼���������л��� ����ɾ���˲���hash���е����ݵ���ͳ�ƴ���
			assert(0);
			break;
		}
		if((ele->key_size == key_size ) && (memcmp(ele->key, key, key_size) == 0))
		{
			return ele;
		}
	}
	return NULL;
}

tlibc_hash_head_t* tlibc_hash_find(tlibc_hash_t *self, const char *key, tuint32 key_size)
{
	tuint32 key_hash = tlibc_hash_key(key, key_size);
	tuint32 key_index = key_hash % self->size;
	const tlibc_hash_bucket_t *bucket = &self->buckets[key_index];
	TLIBC_LIST_HEAD *iter;
	tuint32 i;
	for(iter = bucket->data_list.next, i = 0; iter != &bucket->data_list; iter = iter->next, ++i)
	{
		tlibc_hash_head_t *ele = TLIBC_CONTAINER_OF(iter, tlibc_hash_head_t, data_list);
		if(i >= bucket->data_list_num)
		{
			//���ִ�е���� ˵��������˵�����صĴ��� �����ڴ�Խ�絼���������л��� ����ɾ���˲���hash���е����ݵ���ͳ�ƴ���
			assert(0);
			break;
		}
		if((ele->key_size == key_size ) && (memcmp(ele->key, key, key_size) == 0))
		{
			return ele;
		}
	}
	return NULL;
}

void tlibc_hash_remove(tlibc_hash_t *self, tlibc_hash_head_t *ele)
{
	if(ele->key_index < self->size)
	{
		tlibc_hash_bucket_t		*bucket = &self->buckets[ele->key_index];
		tlibc_list_del(&ele->data_list);
		--bucket->data_list_num;

		if(bucket->data_list_num == 0)
		{
			tlibc_list_del(&bucket->used_bucket_list);
		}
	}
}

void tlibc_hash_clear(tlibc_hash_t *self)
{
	TLIBC_LIST_HEAD *iter;
	tuint32 i;
	for(iter = self->used_bucket_list.next, i = 0; iter != &self->used_bucket_list; iter = iter->next, ++i)
	{
		tlibc_hash_bucket_t *bucket = TLIBC_CONTAINER_OF(iter, tlibc_hash_bucket_t, used_bucket_list);
		if(i >= self->size)
		{
			//���ִ�е���� ˵��������˵�����صĴ��� �����ڴ�Խ�絼���������л��� ����ɾ���˲���hash���е����ݵ���ͳ�ƴ���
			assert(0);
			break;
		}

		init_tlibc_list_head(&bucket->data_list);
		bucket->data_list_num = 0;
	}
	init_tlibc_list_head(&self->used_bucket_list);
}