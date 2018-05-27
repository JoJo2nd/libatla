/********************************************************************

        filename: 	atla_context.c

        Copyright (c) 23:10:2012 James Moran

        This software is provided 'as-is', without any express or implied
        warranty. In no event will the authors be held liable for any damages
        arising from the use of this software.

        Permission is granted to anyone to use this software for any purpose,
        including commercial applications, and to alter it and redistribute it
        freely, subject to the following restrictions:

        1. The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software
        in a product, an acknowledgment in the product documentation would be
        appreciated but is not required.

        2. Altered source versions must be plainly marked as such, and must not
be
        misrepresented as being the original software.

        3. This notice may not be removed or altered from any source
        distribution.

*********************************************************************/

#include "atla/atla_context.h"
#include "atla/atla_memhandler.h"
#include "atla/atla_schema.h"
#include "atla/atla.h"
#include <string.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atAtlaContext_t* ATLA_API atCreateAtlaContext(atMemoryHandler_t* mem) {
  atAtlaContext_t* ctx = mem->memAlloc_(sizeof(atAtlaContext_t), mem->memUser_);
  if (!ctx) return NULL;

  ctx->memCtx_ = mem;
  ctx->schemaCount_ = 0;
  ctx->schemaHead_ = NULL;

  return ctx;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atDestroyAtlaContext(atAtlaContext_t* ctx) {
  atMemoryHandler_t* mem;
  atDataSchema_t *   i, *n;

  if (!ctx) return ATLA_EOK;

  mem = ctx->memCtx_;
  for (i = ctx->schemaHead_; i; i = n) {
    n = i->next_;
    mem->memFree_(i, mem->memUser_);
  }

  mem->memFree_(ctx, mem->memUser_);

  return ATLA_EOK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atErrorCode ATLA_API atAddDataSchema(atAtlaContext_t* ctx,
                                     atDataSchema_t*  schema) {
  atMemoryHandler_t* mem = ctx->memCtx_;
  atDataSchema_t *   i, *newschema, *nestedschema;
  atSchemaElement_t* elements;
  atchar *           strbuf, *strp;
  atuint             eleIdx;
  atuint             memsize;
  atuint32           serialiseSize;

  for (i = ctx->schemaHead_; i; i = i->next_) {
    if (schema->id_ == i->id_) {
      return ATLA_EOK;
    }
  }

  serialiseSize = 0;
  memsize = sizeof(atDataSchema_t);
  memsize += strlen(schema->name_) + 1; //+1 for NULL
  for (eleIdx = 0; eleIdx < schema->elementCount_; ++eleIdx) {
    memsize += sizeof(atSchemaElement_t);
    memsize += strlen(schema->elementArray_[eleIdx].name_) + 1; //+1 for NULL
    if (schema->elementArray_[eleIdx].nestedID_ == 0) {
      serialiseSize += schema->elementArray_[eleIdx].size_ *
                       schema->elementArray_[eleIdx].arrayCount_;
    } else {
      nestedschema =
        atContextGetDataSchema(ctx, schema->elementArray_[eleIdx].nestedID_);
      if (!nestedschema) return ATLA_ENESTEDSCHEMANOTFOUND;
      serialiseSize += nestedschema->serialisedSize_ *
                       schema->elementArray_[eleIdx].arrayCount_;
    }
  }

  newschema = mem->memAlloc_(memsize, mem->memUser_);

  if (!newschema) return ATLA_NOMEM;

  elements = (atSchemaElement_t*)(newschema + 1);
  strbuf = strp = (atchar*)(elements + schema->elementCount_);

  *newschema = *schema;
  newschema->elementArray_ = elements;
  newschema->name_ = strp;
  newschema->serialisedSize_ = serialiseSize;
  strcpy(newschema->name_, schema->name_);
  strp += strlen(newschema->name_) + 1;

  for (eleIdx = 0; eleIdx < schema->elementCount_; ++eleIdx) {
    elements[eleIdx] = schema->elementArray_[eleIdx];
    elements[eleIdx].name_ = strp;
    strcpy(elements[eleIdx].name_, schema->elementArray_[eleIdx].name_);
    strp += strlen(elements[eleIdx].name_) + 1;
  }

  newschema->next_ = ctx->schemaHead_;
  newschema->prev_ = NULL;
  if (ctx->schemaHead_) ctx->schemaHead_->prev_ = newschema;

  ctx->schemaHead_ = newschema;
  ++ctx->schemaCount_;

  return ATLA_EOK;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

atDataSchema_t* ATLA_API atContextGetDataSchema(atAtlaContext_t* ctx,
                                                atUUID_t         id) {
  atDataSchema_t* i;

  for (i = ctx->schemaHead_; i; i = i->next_) {
    if (i->id_ == id) {
      return i;
    }
  }

  return NULL;
}
