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

#include <libbuild2/config/utility.hxx>
#include <libbuild2/diagnostics.hxx>
#include <libbuild2/file.hxx>
#include <libbuild2/protobuf/init.hpp>
#include <libbuild2/protobuf/module.hpp>
#include <libbuild2/protobuf/rule.hpp>
#include <libbuild2/protobuf/target.hpp>
#include <libbuild2/scope.hxx>
#include <libbuild2/target.hxx>
#include <libbuild2/variable.hxx>

namespace build2::protobuf {

bool guess_init(scope &root_scope, scope &base_scope, const location &location,
                bool, bool opt, module_init_extra &extra) {
  tracer trace("protobuf::guess_init");
  l5([&] { trace << "for " << root_scope; });

  if (root_scope != base_scope) {
    fail(location) << "protobuf.guess module must be loaded in project root";
  }

  config::save_module(root_scope, "protobuf", 150);

  auto &vp(root_scope.var_pool());

  auto &v_ver(vp.insert<string>("protobuf.version"));
  auto &v_sum(vp.insert<string>("protobuf.checksum"));

  bool new_cfg(false);
  auto ir(import_direct<exe>(
      new_cfg, root_scope, name("protoc", dir_path(), "exe", "protoc"),
      true /* phase2 */, opt, true /* metadata */, location, "module load"));

  const exe *tgt(ir.first);

  auto *ver(tgt != nullptr ? &cast<string>(tgt->vars[v_ver]) : nullptr);
  auto *sum(tgt != nullptr ? &cast<string>(tgt->vars[v_sum]) : nullptr);

  if (verb >= (new_cfg ? 2 : 3)) {
    diag_record dr(text);
    dr << "proto " << project(root_scope) << '@' << root_scope << '\n';

    if (tgt != nullptr)
      dr << "  proto      " << ir << '\n'
         << "  version    " << *ver << '\n'
         << "  checksum   " << *sum;
    else
      dr << "  proto      "
         << "not found, leaving unconfigured";
  }

  if (tgt == nullptr)
    return false;

  root_scope.assign("proto") = ir.first->name;
  root_scope.assign(v_sum) = *sum;
  root_scope.assign(v_ver) = *ver;

  {
    standard_version v(*ver);

    root_scope.assign<uint64_t>("proto.version.number") = v.version;
    root_scope.assign<uint64_t>("proto.version.major") = v.major();
    root_scope.assign<uint64_t>("proto.version.minor") = v.minor();
    root_scope.assign<uint64_t>("proto.version.patch") = v.patch();
  }

  extra.set_module(new module(data{*tgt, *sum}));

  return true;
}

bool config_init(scope &root_scope, scope &base_scope, const location &location,
                 bool, bool opt, module_init_extra &extra) {
  tracer trace("protobuf::config_init");
  l5([&] { trace << "for " << root_scope; });

  if (root_scope != base_scope) {
    fail(location) << "protobuf.config module must be loaded in project root";
  }

  if (optional<shared_ptr<build2::module>> r =
          load_module(root_scope, root_scope, "protobuf.guess", location, opt,
                      extra.hints)) {
    extra.module = *r;
  } else {
    if (!opt)
      fail(location) << "protobuf could not be configured" << info
                     << "re-run with -V for more information";

    return false;
  }

  using config::append_config;
  append_config<strings>(root_scope, root_scope, "proto.options", nullptr);

  return true;
}

bool init(scope &root_scope, scope &base_scope, const location &location, bool,
          bool opt, module_init_extra &extra) {
  tracer trace("protobuf::init");
  l5([&] { trace << "for " << root_scope; });

  if (root_scope != base_scope) {
    fail(location) << "protobuf module must be loaded in project root";
  }

  if (!cast_false<bool>(root_scope["cxx.loaded"])) {
    fail(location) << "cxx module must be loaded before protobuf";
  }

  if (optional<shared_ptr<build2::module>> r =
          load_module(root_scope, root_scope, "protobuf.config", location, opt,
                      extra.hints)) {
    extra.module = *r;
  } else {
    if (!opt)
      fail(location) << "protobuf could not be configured" << info
                     << "re-run with -V for more information";

    return false;
  }

  auto &m(extra.module_as<module>());

  root_scope.insert_target_type<proto>();
  root_scope.insert_target_type<proto_cxx>();

  {
    auto reg = [&root_scope, &m](meta_operation_id mid, operation_id oid) {
      root_scope.insert_rule<proto_cxx>(mid, oid, "protobuf.compile", m);
      root_scope.insert_rule<cxx::hxx>(mid, oid, "protobuf.compile", m);
      root_scope.insert_rule<cxx::cxx>(mid, oid, "protobuf.compile", m);
    };

    reg(perform_id, update_id);
    reg(perform_id, clean_id);

    reg(configure_id, update_id);
    reg(dist_id, update_id);
  }

  return true;
}

const module_functions *build2_protobuf_load() {
  static constexpr module_functions module_functions[]{
      {"protobuf.guess", nullptr, guess_init},
      {"protobuf.config", nullptr, config_init},
      {"protobuf", nullptr, init},
      {nullptr, nullptr, nullptr},
  };

  return module_functions;
}

} // namespace build2::protobuf
