#pragma once

#include "storage/page/page.h"
#include "storage/table/tmp_tuple.h"
#include "storage/table/tuple.h"

namespace bustub {

// To pass the test cases for this class, you must follow the existing TmpTuplePage format and implement the
// existing functions exactly as they are! It may be helpful to look at TablePage.
// Remember that this task is optional, you get full credit if you finish the next task.
// 要通过此类的测试用例，您必须遵循现有的 TmpTuplePage 格式并实现
// 现有函数与它们原样一样！查看 TablePage 可能会有所帮助。
// 请记住，此任务是可选的，如果完成下一个任务，您将获得满分。

/**
 * TmpTuplePage format:
 *
 * Sizes are in bytes.
 * | PageId (4) | LSN (4) | FreeSpace (4) | (free space) | TupleSize2 | TupleData2 | TupleSize1 | TupleData1 |
 *
 * We choose this format because DeserializeExpression expects to read Size followed by Data.
 * 我们选择这种格式是因为 DeserializeExpression 期望读取 Size，然后读取 Data。
 */
class TmpTuplePage : public Page {
 public:
  void Init(page_id_t page_id, uint32_t page_size) {}

  page_id_t GetTablePageId() { return INVALID_PAGE_ID; }

  bool Insert(const Tuple &tuple, TmpTuple *out) { return false; }

 private:
  static_assert(sizeof(page_id_t) == 4);
};

}  // namespace bustub
