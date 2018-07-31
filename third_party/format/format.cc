/*  Derived from the OCaml Compatibility Library for F# (Format module)
    (FSharp.Compatibility.OCaml.Format)

    Copyright (c) 1996  Institut National de Recherche en
                        Informatique et en Automatique
    Copyright (c) Jack Pappas 2012
        http://github.com/fsprojects

    This code is distributed under the terms of the
    GNU Lesser General Public License (LGPL) v2.1.
    See the LICENSE file for details. */

#include "third_party/format/format.h"

namespace ocaml {
namespace format {
void Formatter::BreakNewLine(int offset, int width) {
  OutputNewline();
  is_new_line_ = true;
  int indent = margin_ - width + offset;
  // Don't indent more than max_indent_.
  int real_indent = max_indent_ < indent ? max_indent_ : indent;
  current_indent_ = real_indent;
  space_left_ = margin_ - current_indent_;
  OutputSpaces(current_indent_);
}
void Formatter::ForceBreakLine() {
  if (format_stack_.empty()) {
    OutputNewline();
  } else {
    const auto& top = format_stack_.top();
    if (top.width > space_left_) {
      if (top.block_type != Token::BlockType::Fits &&
          top.block_type != Token::BlockType::HBox) {
        BreakLine(top.width);
      }
    }
  }
}
void Formatter::AdvanceLoop() {
  while (!queue_.empty()) {
    const auto& token = queue_.front();
    if (!(token.elem_size < 0 && right_total_ - left_total_ < space_left_)) {
      FormatToken(token.elem_size < 0 ? kInfinity : token.elem_size,
                  token.token);
      left_total_ += token.length;
      queue_.pop_front();
    } else {
      return;
    }
  }
}
void Formatter::SetSize(bool ty) {
  if (scan_stack_.empty()) {
    return;
  }
  auto* queue_elem = scan_stack_.top().queue_elem;
  int left_tot = scan_stack_.top().left_total;
  int size = queue_elem->elem_size;
  const auto& tok = queue_elem->token;
  if (left_tot < left_total_) {
    ClearScanStack();
  } else if ((ty && (tok.kind == Token::Kind::Break ||
                     tok.kind == Token::Kind::TBreak)) ||
             (!ty && tok.kind == Token::Kind::Begin)) {
    queue_elem->elem_size = right_total_ + size;
    scan_stack_.pop();
  }
}
void Formatter::FormatToken(int size, const Token& token) {
  switch (token.kind) {
    case Token::Kind::Text: {
      space_left_ -= size;
      OutputString(token.text);
      is_new_line_ = false;
    } break;
    case Token::Kind::Begin: {
      int off = token.int_1;
      Token::BlockType ty = token.block_type;
      int insertion_point = margin_ - space_left_;
      if (insertion_point > max_indent_) {
        ForceBreakLine();
      }
      int offset = space_left_ - off;
      Token::BlockType bl_type = Token::BlockType::VBox;
      if (ty != Token::BlockType::VBox) {
        bl_type = size > space_left_ ? ty : Token::BlockType::Fits;
      }
      format_stack_.emplace(FormatElem{bl_type, offset});
    } break;
    case Token::Kind::End: {
      if (!format_stack_.empty()) {
        format_stack_.pop();
      }
    } break;
    case Token::Kind::TBegin: {
      tbox_stack_.emplace(std::move(token.tbox));
    } break;
    case Token::Kind::TEnd: {
      if (!tbox_stack_.empty()) {
        tbox_stack_.pop();
      }
    } break;
    case Token::Kind::STab: {
      if (!tbox_stack_.empty()) {
        auto& tabs = tbox_stack_.top();
        bool added = false;
        int n = margin_ - space_left_;
        for (auto& tab : tabs) {
          if (n < tab) {
            tab = n;
            added = true;
            break;
          }
        }
        if (!added) {
          tabs.push_back(n);
        }
      }
    } break;
    case Token::Kind::TBreak: {
      if (!tbox_stack_.empty()) {
        int n = token.int_1;
        int off = token.int_2;
        int insertion_point = margin_ - space_left_;
        auto& tabs = tbox_stack_.top();
        int tab;
        if (tabs.empty()) {
          tab = insertion_point;
        } else {
          tab = tabs[0];
          for (auto& x : tabs) {
            if (x >= insertion_point) {
              tab = x;
              break;
            }
          }
        }
        int offset = tab - insertion_point;
        if (offset >= 0) {
          BreakSameLine(offset + n);
        } else {
          BreakNewLine(tab + off, margin_);
        }
      }
    } break;
    case Token::Kind::Newline: {
      if (!format_stack_.empty()) {
        BreakLine(format_stack_.top().width);
      }
    } break;
    case Token::Kind::IfNewline: {
      if (current_indent_ != margin_ - space_left_) {
        SkipToken();
      }
    } break;
    case Token::Kind::Break: {
      if (!format_stack_.empty()) {
        int n = token.int_1;
        int off = token.int_2;
        int width = format_stack_.top().width;
        switch (format_stack_.top().block_type) {
          case Token::BlockType::HOVBox:
            if (size > space_left_) {
              BreakNewLine(off, width);
            } else {
              BreakSameLine(n);
            }
            break;
          case Token::BlockType::Box:
            if (is_new_line_) {
              BreakSameLine(n);
            } else if (size > space_left_) {
              BreakNewLine(off, width);
            } else if (current_indent_ > margin_ - width + off) {
              BreakNewLine(off, width);
            } else {
              BreakSameLine(n);
            }
            break;
          case Token::BlockType::HVBox:
            BreakNewLine(off, width);
            break;
          case Token::BlockType::Fits:
            BreakSameLine(n);
            break;
          case Token::BlockType::VBox:
            BreakNewLine(off, width);
            break;
          case Token::BlockType::HBox:
            BreakSameLine(n);
            break;
        }
      }
    } break;
    case Token::Kind::OpenTag: {
      std::string marker = stream_->MarkOpenTag(token.text);
      OutputString(marker);
      mark_stack_.push(marker);
    } break;
    case Token::Kind::CloseTag: {
      if (!mark_stack_.empty()) {
        std::string marker = stream_->MarkCloseTag(mark_stack_.top());
        OutputString(marker);
        mark_stack_.pop();
      }
    } break;
  }
}
void Formatter::RInit() {
  ClearQueue();
  ClearScanStack();
  format_stack_ = std::stack<FormatElem>();
  tbox_stack_ = std::stack<Token::TBlock>();
  tag_stack_ = std::stack<std::string>();
  mark_stack_ = std::stack<std::string>();
  current_indent_ = 0;
  curr_depth_ = 0;
  space_left_ = margin_;
  OpenSysBox();
}
void Formatter::set_margin(int n) {
  if (n >= 1) {
    n = limit(n);
    margin_ = n;
    int new_max_indent;
    if (max_indent_ < margin_) {
      new_max_indent = max_indent_;
    } else {
      int half_margin = margin_ / 2;
      int interval = margin_ - min_space_left_;
      new_max_indent = half_margin > interval ? half_margin : interval;
      if (new_max_indent < 1) {
        new_max_indent = 1;
      }
    }
    set_max_indent(new_max_indent);
  }
}
}  // namespace format
}  // namespace ocaml
