#include "core/io/csv_reader.h"

#include <stdexcept>

namespace perception::core::io {

std::shared_ptr<model::IDataSet> CsvReader::readCurves(
    const std::string& path, const ReadOptions& /*opts*/)
{
    // TODO(M2): 解析 .csv（表头列名 + 数据行）。
    throw std::runtime_error("csv: reader not implemented yet (M2): " + path);
}

void registerCsvReader()
{
    ReaderRegistry::instance().registerReader(std::make_shared<CsvReader>());
}

} // namespace perception::core::io
