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

#include <libbuild2/protobuf/target.hpp>

namespace build2::protobuf {

extern const char proto_ext_def[] = "proto";

const target_type proto::static_type{
    "proto",
    &file::static_type,
    &target_factory<proto>,
    nullptr,
    &target_extension_var<proto_ext_def>,
    &target_pattern_var<proto_ext_def>,
    nullptr,
    &file_search,
    false,
};

group_view proto_cxx::group_members(action) const {
  static_assert(sizeof(proto_cxx_members) == sizeof(const target *) * 2,
                "member layout incompatible with array");

  return hxx != nullptr
             ? group_view{reinterpret_cast<const target *const *>(&hxx), 2U}
             : group_view{nullptr, 0};
}

static target *proto_cxx_factory(context &context, const target_type &,
                                 dir_path d, dir_path o, string n) {
  tracer trace("protobuf::proto_cxx_factory");

  context.targets.insert<cxx::hxx>(d, o, n, trace).ext("pb.h");
  context.targets.insert<cxx::cxx>(d, o, n, trace).ext("pb.cc");

  return new proto_cxx{context, move(d), move(o), move(n)};
}

const target_type proto_cxx::static_type{
    "proto.cxx",
    &mtime_target::static_type,
    &proto_cxx_factory,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    &target_search,
    true,
};

} // namespace build2::protobuf
