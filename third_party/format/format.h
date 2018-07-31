/*  Derived from the OCaml Compatibility Library for F# (Format module)
    (FSharp.Compatibility.OCaml.Format)

    Copyright (c) 1996  Institut National de Recherche en
                        Informatique et en Automatique
    Copyright (c) Jack Pappas 2012
        http://github.com/fsprojects

    This code is distributed under the terms of the
    GNU Lesser General Public License (LGPL) v2.1.
    See the LICENSE file for details. */

#ifndef THIRD_PARTY_FORMAT_FORMAT_H_
#define THIRD_PARTY_FORMAT_FORMAT_H_

#include "absl/strings/string_view.h"

#include <inttypes.h>
#include <limits>
#include <sstream>
#include <stack>
#include <string>
#include <vector>

namespace ocaml {
namespace format {

using Size = int;

struct Token {
  enum class Kind {
    Text,       ///< normal text
    Break,      ///< complete break
    TBreak,     ///< Go to next tabulation
    STab,       ///< Set a tabulation
    Begin,      ///< Begin a block
    End,        ///< End a block
    TBegin,     ///< Begin a tabulation block
    TEnd,       ///< End a tabulation block
    Newline,    ///< Force a newline inside a block
    IfNewline,  ///< Do something only if this very line has been broken
    OpenTag,    ///< Open atag name
    CloseTag    ///< Close the most recently opened tag
  };
  enum class BlockType {
    HBox,    ///< Horizontal block with no line breaking
    VBox,    ///< Vertical block; each break is a new line
    HVBox,   ///< Horizontal-vertical; same as VBox unless the contents
             ///< fit on a single line
    HOVBox,  ///< Horizontal or vertical; breaks lead to newlines only
             ///< when necessary to print the content of the block
    Box,     ///< Horizontal or indent; breaks lead to newlines only
             ///< when necessary to print the contents of the block, or
             ///< when it leads to a new indentation of the current line
    Fits,    ///< Internal: when a block fits on a single line
  };
  static Token Text(absl::string_view s) {
    Token t{Kind::Text};
    t.text = std::string(s);
    return t;
  }
  static Token Break(int width, int offset) {
    Token t{Kind::Break};
    t.int_1 = width;
    t.int_2 = offset;
    return t;
  }
  static Token TBreak(int width, int offset) {
    Token t{Kind::TBreak};
    t.int_1 = width;
    t.int_2 = offset;
    return t;
  }
  static Token STab() {
    Token t{Kind::STab};
    return t;
  }
  static Token Begin(int indent, BlockType block_type) {
    Token t{Kind::Begin};
    t.int_1 = indent;
    t.block_type = block_type;
    return t;
  }
  static Token End() {
    Token t{Kind::End};
    return t;
  }
  static Token TBegin() {
    Token t{Kind::TBegin};
    return t;
  }
  static Token TEnd() {
    Token t{Kind::TEnd};
    return t;
  }
  static Token Newline() {
    Token t{Kind::Newline};
    return t;
  }
  static Token IfNewline() {
    Token t{Kind::IfNewline};
    return t;
  }
  static Token OpenTag(absl::string_view tag_name) {
    Token t{Kind::OpenTag};
    t.text = std::string(tag_name);
    return t;
  }
  static Token CloseTag() {
    Token t{Kind::CloseTag};
    return t;
  }
  Kind kind;
  /// For Text and OpenTag.
  std::string text;
  /// For Break, TBreak, and Begin.
  int int_1;
  /// For Break and TBreak.
  int int_2;
  /// For Begin.
  BlockType block_type;
  /// For TBegin.
  using TBlock = std::vector<int>;
  TBlock tbox;
};

struct QueueElem {
  /// Set when the size of the block is known.
  Size elem_size;
  /// The declared length of the token.
  int length;
  /// The token.
  Token token;
};

struct ScanElem {
  /// The value of left_total when the element was enqueued.
  int left_total;
  /// The queue element on the stack.
  QueueElem* queue_elem;
};

/// Used to break lines while printing tokens.
struct FormatElem {
  Token::BlockType block_type;
  int width;
};

class FormatterOutputStream {
 public:
  virtual ~FormatterOutputStream() {}
  /// Output function.
  virtual void String(absl::string_view string) = 0;
  /// Flushing function.
  virtual void Flush() = 0;
  /// Output of new lines.
  virtual void Newline() = 0;
  /// Output of indentation spaces.
  virtual void Spaces(int count) = 0;
  virtual std::string MarkOpenTag(absl::string_view tag) {
    return std::string(tag);
  }
  virtual std::string MarkCloseTag(absl::string_view tag) {
    return std::string(tag);
  }
  virtual void PrintOpenTag(absl::string_view tag) {}
  virtual void PrintCloseTag(absl::string_view tag) {}
};

class StringStream : public FormatterOutputStream {
 public:
  void String(absl::string_view string) override { data_ << string; }
  void Flush() override {}
  void Newline() override { data_ << "\n"; }
  void Spaces(int count) override {
    while (count > 0) {
      --count;
      data_ << " ";
    }
  }
  /// Returns the `string` printed to thus far.
  std::string str() { return data_.str(); }

 private:
  std::stringstream data_;
};

class Formatter {
 public:
  Formatter(FormatterOutputStream* stream) : stream_(stream) {
    queue_.emplace_front(
        QueueElem{-1, 0, Token::Begin(0, Token::BlockType::HOVBox)});
    scan_stack_.emplace(ScanElem{1, &queue_.front()});
  }
  ~Formatter() { FlushQueue(false); }
  bool dump_ranges() const { return dump_ranges_; }
  void set_dump_ranges(bool dump_ranges) { dump_ranges_ = dump_ranges; }

 private:
  void Enqueue(const QueueElem&& token) {
    right_total_ += token.length;
    queue_.emplace_back(token);
  }
  void ClearQueue() {
    left_total_ = 1;
    right_total_ = 1;
    queue_.clear();
  }
  void OutputString(absl::string_view string) { stream_->String(string); }
  void OutputNewline() { stream_->Newline(); }
  void OutputSpaces(int count) { stream_->Spaces(count); }
  /// To format a break, indenting a new line.
  void BreakNewLine(int offset, int width);
  /// To force a line break inside a block: no offset is added.
  void BreakLine(int width) { BreakNewLine(0, width); }
  /// To format a break that fits on the current line.
  void BreakSameLine(int width) {
    space_left_ -= width;
    OutputSpaces(width);
  }
  /// To indent no more than max_indent_, if one tries to open a block
  /// beyond max_indent_, then the block is rejected on the left
  /// by simulating a break.
  void ForceBreakLine();
  /// To skip a token, if the previous line has been broken.
  void SkipToken() {
    if (queue_.empty()) {
      return;
    }
    const auto& token = queue_.front();
    left_total_ -= token.length;
    space_left_ += token.elem_size;
    queue_.pop_front();
  }
  void FormatToken(int size, const Token& token);
  void AdvanceLoop();
  void EnqueueAdvance(QueueElem&& tok) {
    Enqueue(std::move(tok));
    AdvanceLoop();
  }
  void EnqueueStringAs(int size, absl::string_view s) {
    if (size > 0) {
      EnqueueAdvance(QueueElem{size, size, Token::Text(s)});
    }
  }
  void EnqueueString(absl::string_view s) { EnqueueStringAs((int)s.size(), s); }
  void ClearScanStack() {
    scan_stack_ = std::stack<ScanElem>();
    scan_stack_.emplace(ScanElem{-1, &scan_stack_bottom_});
  }
  /// Set size of blocks on scan stack :
  /// if ty = true then size of break is set else size of block is set;
  /// in each case pp_scan_stack is popped.
  void SetSize(bool ty);
  /// Push a token on scan stack. If b is true set_size is called.
  void ScanPush(bool b, QueueElem&& tok) {
    Enqueue(std::move(tok));
    if (b) {
      SetSize(true);
    }
    scan_stack_.emplace(ScanElem{right_total_, &queue_.back()});
  }
  void OpenBoxGen(int indent, Token::BlockType br_ty) {
    ++curr_depth_;
    if (curr_depth_ < max_boxes_) {
      ScanPush(false, QueueElem{-right_total_, 0, Token::Begin(indent, br_ty)});
    } else if (curr_depth_ == max_boxes_) {
      EnqueueString(ellipsis_);
    }
  }
  void OpenSysBox() { OpenBoxGen(0, Token::BlockType::HOVBox); }
  void RInit();
  void PrintAsSize(int size, absl::string_view s) {
    if (curr_depth_ < max_boxes_) {
      EnqueueStringAs(size, s);
    }
  }
  void PrintAs(int isize, absl::string_view s) { PrintAsSize(isize, s); }

 public:
  void FlushQueue(bool newline) {
    while (curr_depth_ > 1) {
      CloseBox();
    }
    right_total_ = kInfinity;
    AdvanceLoop();
    if (newline) {
      OutputNewline();
    }
    RInit();
  }
  void PrintString(absl::string_view s) { PrintAs((int)s.size(), s); }
  void CloseBox() {
    if (curr_depth_ > 1) {
      if (curr_depth_ < max_boxes_) {
        Enqueue(QueueElem{0, 0, Token::End()});
        SetSize(true);
        SetSize(false);
      }
      --curr_depth_;
    }
  }
  void OpenTag(absl::string_view tag_name) {
    if (print_tags_) {
      tag_stack_.push(std::string(tag_name));
      stream_->PrintOpenTag(tag_name);
    }
    if (mark_tags_) {
      Enqueue(QueueElem{0, 0, Token::OpenTag(tag_name)});
    }
  }
  void CloseTag(absl::string_view tag_name) {
    if (print_tags_) {
      if (!tag_stack_.empty()) {
        stream_->PrintCloseTag(tag_stack_.top());
        tag_stack_.pop();
      }
    }
    if (mark_tags_) {
      Enqueue(QueueElem{0, 0, Token::CloseTag()});
    }
  }
  void OpenHBox() { OpenBoxGen(0, Token::BlockType::HBox); }
  void OpenVBox(int indent) { OpenBoxGen(indent, Token::BlockType::VBox); }
  void OpenHVBox(int indent) { OpenBoxGen(indent, Token::BlockType::HVBox); }
  void OpenHOVBox(int indent) { OpenBoxGen(indent, Token::BlockType::HOVBox); }
  void OpenBox(int indent) { OpenBoxGen(indent, Token::BlockType::Box); }
  /// Print a newline after printing all queued text.
  void PrintNewline() {
    FlushQueue(true);
    stream_->Flush();
  }
  void PrintFlush() {
    FlushQueue(false);
    stream_->Flush();
  }
  /// To get a newline when one does not want to close the current block.
  void ForceNewline() {
    if (curr_depth_ < max_boxes_) {
      EnqueueAdvance(QueueElem{0, 0, Token::Newline()});
    }
  }
  /// To format something if the line has just been broken.
  void PrintIfNewline() {
    if (curr_depth_ < max_boxes_) {
      EnqueueAdvance(QueueElem{0, 0, Token::IfNewline()});
    }
  }
  /// Breaks: indicate where a block may be broken.
  /// If line is broken then offset is added to the indentation of the current
  /// block else (the value of) width blanks are printed.
  void PrintBreak(int width, int offset) {
    if (curr_depth_ < max_boxes_) {
      ScanPush(true,
               QueueElem{-right_total_, width, Token::Break(width, offset)});
    }
  }
  void PrintSpace() { PrintBreak(1, 0); }
  void PrintCut() { PrintBreak(0, 0); }
  void OpenTBox() {
    ++curr_depth_;
    if (curr_depth_ < max_boxes_) {
      EnqueueAdvance(QueueElem{0, 0, Token::TBegin()});
    }
  }
  void CloseTBox() {
    if (curr_depth_ > 1 && curr_depth_ < max_boxes_) {
      EnqueueAdvance(QueueElem{0, 0, Token::TEnd()});
      --curr_depth_;
    }
  }
  void PrintTBreak(int width, int offset) {
    if (curr_depth_ < max_boxes_) {
      ScanPush(true,
               QueueElem{-right_total_, width, Token::TBreak(width, offset)});
    }
  }
  void PrintTab() { PrintTBreak(0, 0); }
  void SetTab() { EnqueueAdvance(QueueElem{0, 0, Token::STab()}); }
  void set_max_boxes(int n) {
    if (n > 1) {
      max_boxes_ = n;
    }
  }
  int max_boxes() const { return max_boxes_; }
  bool over_max_boxes() const { return curr_depth_ == max_boxes_; }
  void set_ellipsis_text(absl::string_view text) {
    ellipsis_ = std::string(text);
  }
  absl::string_view ellipsis_text() const { return ellipsis_; }
  int limit(int n) {
    if (n < kInfinity) {
      return n;
    } else {
      return kInfinity - 1;
    }
  }
  void set_min_space_left(int n) {
    if (n >= 1) {
      n = limit(n);
      min_space_left_ = n;
      max_indent_ = margin_ - min_space_left_;
      RInit();
    }
  }
  void set_max_indent(int n) { set_min_space_left(margin_ - n); }
  void set_margin(int n);
  int margin() const { return margin_; }
  const void* last_range() const { return last_range_; }
  void set_last_range(const void* cookie) { last_range_ = cookie; }

 private:
  std::stack<ScanElem> scan_stack_;
  std::stack<FormatElem> format_stack_;
  std::stack<Token::TBlock> tbox_stack_;
  std::stack<std::string> tag_stack_;
  std::stack<std::string> mark_stack_;
  std::deque<QueueElem> queue_;
  QueueElem scan_stack_bottom_ = QueueElem{-1, 0, Token::Text("")};
  static constexpr int kInfinity = 1000000010;
  /// Value of right margin.
  int margin_ = 78;
  /// Minimal space left before margin, when opening a block.
  int min_space_left_ = 10;
  /// Maximum value of indentation: no blocks can be opened further.
  int max_indent_ = 78 - 10;
  /// Space remaining on the current line.
  int space_left_ = 78;
  /// Current value of indentation.
  int current_indent_ = 0;
  /// True when the line has been broken by the pretty-printer.
  bool is_new_line_ = true;
  /// Total width of tokens already printed.
  int left_total_ = 1;
  /// Total width of tokens ever put in queue.
  int right_total_ = 1;
  /// Current number of opened blocks.
  int curr_depth_ = 1;
  /// Maximum number of blocks which can be simultaneously opened.
  int max_boxes_ = std::numeric_limits<int>::max();
  /// Ellipsis string.
  std::string ellipsis_ = ".";
  /// Output stream.
  FormatterOutputStream* stream_;
  /// Are tags printed?
  bool print_tags_ = false;
  /// Are tags marked?
  bool mark_tags_ = false;
  /// Should we show ranges?
  bool dump_ranges_ = true;
  /// An identifier for the last range shown.
  const void* last_range_ = nullptr;
};

}  // namespace format
}  // namespace ocaml

#endif  // THIRD_PARTY_FORMAT_FORMAT_H_
