#include "Buffer.hpp"

#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace sjtu {

Buffer::Buffer(const std::filesystem::path& path) : path_(path) {
    //从path指向的文件构造Buffer,你需要打开文件并且把文件内容填充进Buffer,并正确初始化一些状态.
    //注意path可能为空的边界情况
    //文件不存在或内容为空时,lines_里也要留下一个空string作为占位
    if (!path.empty()) {
        std::ifstream file(path.string());
        std::string line;
        while (std::getline(file, line)) {
            lines_.push_back(line);
        }
    }

    if (lines_.empty()) {
        lines_.push_back("");
    }
}

Buffer::Buffer(std::vector<std::string> lines, std::filesystem::path path) {
    lines_ = std::move(lines);
    path_ = std::move(path);
    if (lines_.empty()) {
        lines_.push_back("");
    }
}

std::size_t Buffer::GetLineCount() const {
    //返回文件行数
    return lines_.size();
}

const std::string& Buffer::GetLineAt(std::size_t row) const {
    //返回第row行的内容(row从0开始)
    if (row >= lines_.size()) {
        throw std::out_of_range("Buffer::GetLineAt: row out of range");
    }
    return lines_[row];
}

std::string Buffer::GetDisplayName() const {
    //返回文件名,若是新文件,返回"[No Name]"
    if (path_.empty()) {
        return "[No Name]";
    }
    return path_.filename().string();
}

bool Buffer::IsModified() const {
    //返回文件和上次保存比起来是否被修改过
    return modified_;
}

void Buffer::InsertCharacter(std::size_t row, std::size_t column, char value) {
    //在第row行第column列插入一个value, 注意越界检查
    if (row >= lines_.size() || column > lines_[row].size()) {
        throw std::out_of_range("Buffer::InsertCharacter: position out of range");
    }
    lines_[row].insert(column, 1, value);
    modified_ = true;
}

void Buffer::EraseCharacter(std::size_t row, std::size_t column) {
    //在第row行第column列删除一个字符
    if (row >= lines_.size() || column >= lines_[row].size()) {
        throw std::out_of_range("Buffer::EraseCharacter: position out of range");
    }
    lines_[row].erase(column, 1);
    modified_ = true;
}

void Buffer::SplitLine(std::size_t row, std::size_t column) {
    //在第row行第column列分割,即在此处敲了回车键
    if (row >= lines_.size() || column > lines_[row].size()) {
        throw std::out_of_range("Buffer::SplitLine: position out of range");
    }
    std::string tail = lines_[row].substr(column);
    lines_[row].erase(column);
    lines_.insert(lines_.begin() + static_cast<std::ptrdiff_t>(row) + 1, std::move(tail));
    modified_ = true;
}

void Buffer::JoinLine(std::size_t row) {
    //把第row + 1行合并进第row行
    if (row + 1 >= lines_.size()) {
        return;
    }
    lines_[row] += lines_[row + 1];
    lines_.erase(lines_.begin() + static_cast<std::ptrdiff_t>(row) + 1);
    modified_ = true;
}

void Buffer::Save() {
    //把文件内容保存, 直接调用WriteTo方法
    if (path_.empty()) {
        throw std::runtime_error("no file name");
    }
    WriteTo(path_);
    modified_ = false;
}

void Buffer::SaveAs(const std::filesystem::path& path) {
    //保存到新的路径,并把这个路径记为当前文件
    if (path.empty()) {
        throw std::runtime_error("no file name");
    }
    WriteTo(path);
    path_ = path;
    modified_ = false;
}

void Buffer::WriteTo(const std::filesystem::path& path) const {
    //实际将缓冲区中的内容写入path指向的文件中
    if (path.empty()) {
        throw std::runtime_error("no file name");
    }

    std::ofstream file(path.string(), std::ios::trunc);
    if (!file.is_open()) {
        throw std::runtime_error("cannot open file: " + path.string());
    }

    //只有"一整行空行"的缓冲区写出一个空文件,否则每行后面都补一个换行符
    if (!(lines_.size() == 1 && lines_[0].empty())) {
        for (const std::string& line : lines_) {
            file << line << endl;
        }
    }

    file.flush();
    if (!file) {
        throw std::runtime_error("cannot write file: " + path.string());
    }
}

} // namespace sjtu
