#include "Renderer.hpp"
#include "TextLayout.hpp"

#include <algorithm>
#include <cstddef>
#include <string>

namespace sjtu {

//我们在Render中保留了所有需要用到控制序列的部分逻辑,你不应该修改它们.
//啥是控制序列?它们是嵌入在文本流中的“指令”,告诉终端执行特定操作(比如移动光标、改变文字颜色、清屏或清除行内容).   
namespace {

void AppendClearedLine(std::string& frame, std::string_view contents, std::size_t width, bool newline) {
    //追加不超过width列的显示内容,再用ESC[K清除这一行余下的旧内容
    //需要换行时同时输出回车和换行,因为raw模式关闭了终端的自动输出转换
    //你在生成帧的时候生成每一行都应该调用这个函数
    frame.append(contents.substr(0, width));
    frame += "\x1b[K";
    if (newline) {
        frame += "\r\n";
    }
}

std::string CursorSequence(std::size_t row, std::size_t column) {
    //生成定位光标的ANSI转义序列,这里接收的行列都从1开始
    return "\x1b[" + std::to_string(row) + ';' + std::to_string(column) + 'H';
}


}

std::string Renderer::Render(const Buffer& buffer, const Window& window, const RenderState& state) const {
    //1. 隐藏光标并回到屏幕左上角,开始拼接新的一帧(一帧就是一个长string)
    //2. 按视口逐行取Buffer内容,展开Tab后再横向截取;超出文件的行显示~
    //3. 绘制底部一行,优先显示命令行,其次是提示信息,最后是Insert模式标记
    //4. 根据当前模式计算光标的屏幕位置,追加定位序列并重新显示光标
    //你应该区分我们的Cursor光标(它代表当前用户在修改文件的哪个位置)和终端显示的白色亮条,白色亮条在屏幕中的位置是由光标位置和视口位置计算的
    const Viewport& viewport = window.GetViewport();
    const std::size_t width = std::max<std::size_t>(viewport.columns_, 1);
    const std::size_t rows = std::max<std::size_t>(viewport.rows_, 1);

    std::string frame;
    frame.reserve((rows + 1) * (width + 8));
    frame += "\x1b[?25l";
    frame += "\x1b[H";

    for (std::size_t screen_row = 0; screen_row < rows; ++screen_row) {
        //这里是提示2
        const std::size_t buffer_row = viewport.top_ + screen_row;
        if (buffer_row < buffer.GetLineCount()) {
            const std::string display_line = ExpandForDisplay(buffer.GetLineAt(buffer_row));
            std::string slice =(viewport.left_<=display_line.size())?display_line.substr(viewport.left_,width):"";
            AppendClearedLine(frame, slice, width, true);
        } else {
            AppendClearedLine(frame, "~", width, true);
        }
    }

    std::string bottom;
    if (state.mode_ == Mode::CommandLine) {
        bottom = ":" + state.command_;
    } else if (!state.message_.empty()) {
        bottom = state.message_;
    } else if (state.mode_ == Mode::Insert) {
        bottom = "-- INSERT --";
    }
    AppendClearedLine(frame, bottom, width, false);
    //这里是提示3中的部分
    //我们只会在CommandMode的时候检查一下底部的命令内容,在NormalMode不会看底部,所以message你可以随意写

    //计算cursor_row和cursor_column即可
    //正文里显示光标时,命令行模式停在底部那一行(冒号后面),其余模式停在当前字符上
    std::size_t cursor_row = rows + 1;
    std::size_t cursor_column = 1;
    if (state.mode_ == Mode::CommandLine) {
        cursor_column = std::min(state.command_.size() + 2, width);
    } else {
        const Position& cursor = window.GetCursor();
        if (cursor.row_ >= viewport.top_) {
            cursor_row = std::min(cursor.row_ - viewport.top_ + 1, rows);
        }
        const std::size_t visual_column =
            BufferColumnToRenderColumn(buffer.GetLineAt(cursor.row_), cursor.column_);
        if (visual_column >= viewport.left_) {
            cursor_column = std::min(visual_column - viewport.left_ + 1, width);
        }
    }

    frame += CursorSequence(cursor_row, cursor_column);
    frame += "\x1b[?25h";
    return frame;
}

std::string Renderer::ExpandForDisplay(std::string_view line) {
    //从左到右扫描buffer中一整行的实际内容,并扩展到render应该输出的视图
    //你应该在Render中调用这个函数,并把函数返回的结果按照视口剪切用于Render的某些行
    std::string result;
    std::size_t current_column = 0;
    for (char value : line) {
        if (value == '\t') {
            result.append(NextScreenColumn(current_column, value) - current_column, ' ');
        } else {
            result += value;
        }
        current_column = NextScreenColumn(current_column, value);
    }
    return result;
}

} // namespace sjtu
