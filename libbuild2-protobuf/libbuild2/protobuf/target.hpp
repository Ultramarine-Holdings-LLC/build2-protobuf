/*
* Copyright © 2022 Ultramarine Holdings LLC
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the “Software”), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*/

#pragma once

#include <libbuild2/cxx/target.hxx>
#include <libbuild2/target-type.hxx>
#include <libbuild2/target.hxx>

namespace build2::protobuf {

class proto : public file {
public:
  using file::file;

  static const target_type static_type;
  virtual const target_type &dynamic_type() const { return static_type; }
};

struct proto_cxx_members {
  const cxx::hxx *hxx{};
  const cxx::cxx *cxx{};
};

class proto_cxx : public mtime_target, public proto_cxx_members {
public:
  using mtime_target::mtime_target;

  virtual group_view group_members(action) const override;

  static const target_type static_type;
  virtual const target_type &dynamic_type() const override {
    return static_type;
  }
};

} // namespace build2::protobuf
