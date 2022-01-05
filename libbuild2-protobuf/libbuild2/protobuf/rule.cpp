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

#include <libbuild2/algorithm.hxx>
#include <libbuild2/depdb.hxx>
#include <libbuild2/protobuf/rule.hpp>
#include <libbuild2/protobuf/target.hpp>

namespace build2::protobuf {

static bool match_stem(const string &name, const string &stem,
                       string *prefix = nullptr, string *suffix = nullptr) {
  size_t p(name.find(stem));

  if (p != string::npos) {
    if (prefix != nullptr)
      prefix->assign(name, 0, p);

    if (suffix != nullptr)
      suffix->assign(name, p + stem.size(), string::npos);

    return true;
  }

  return false;
}

bool compile_rule::match(action action, class target &target,
                         const string &) const {
  tracer trace("protobuf::compile_rule::match");

  auto find = [&trace, action,
               &target](auto &&r) -> optional<prerequisite_member> {
    for (prerequisite_member p : r) {
      if (include(action, target, p) != include_type::normal)
        continue;

      if (p.is_a<proto>()) {
        if (match_stem(target.name, p.name()))
          return p;

        l4([&] {
          trace << ".proto file stem '" << p.name() << "' "
                << "doesn't match target " << target;
        });
      }
    }

    return nullopt;
  };

  if (auto *pt = target.is_a<proto_cxx>()) {
    proto_cxx &t(*pt);

    if (!find(group_prerequisite_members(action, t))) {
      l4([&] { trace << "no .proto source file for target " << t; });
      return false;
    }

    t.hxx = &search<cxx::hxx>(t, t.dir, t.out, t.name);
    t.cxx = &search<cxx::cxx>(t, t.dir, t.out, t.name);

    return true;
  } else {
    const proto_cxx *g(target.ctx.targets.find<proto_cxx>(
        target.dir, target.out, target.name));

    if (g == nullptr || !g->has_prerequisites()) {
      if (optional<prerequisite_member> p =
              find(prerequisite_members(action, target))) {
        if (g == nullptr)
          g = &target.ctx.targets.insert<proto_cxx>(target.dir, target.out,
                                                    target.name, trace);

        g->prerequisites(prerequisites{p->as_prerequisite()});
      }
    }

    if (g == nullptr)
      return false;

    target.group = g;
    return true;
  }

  return false;
}

recipe compile_rule::apply(action action, class target &target) const {
  if (proto_cxx *pt = target.is_a<proto_cxx>()) {
    proto_cxx &t(*pt);

    t.hxx->derive_path();
    t.cxx->derive_path();

    inject_fsdir(action, t);

    match_prerequisite_members(action, t);

    if (action == perform_update_id)
      inject(action, t, this->target);

    switch (action) {
    case perform_update_id:
      return [this](struct action a, const class target &t) {
        return perform_update(a, t);
      };
    case perform_clean_id:
      return &perform_clean_group_depdb;
    default:
      return noop_recipe;
    }
  } else {
    const proto_cxx &g(target.group->as<proto_cxx>());
    build2::match(action, g);
    return group_recipe;
  }
}

static void append_extension(cstrings &args, const path_target &t,
                             const char *option,
                             const char *default_extension) {
  const string *e(t.ext());
  assert(e != nullptr);

  if (*e != default_extension) {
    args.push_back(option);
    args.push_back(e->empty() ? e->c_str() : t.path().extension_cstring() - 1);
  }
}

target_state compile_rule::perform_update(action a,
                                          const class target &xt) const {
  tracer trace("protobuf::compile_rule::perform_update");
  const proto_cxx &t(xt.as<proto_cxx>());
  const path &tp(t.hxx->path());

  timestamp mt(t.load_mtime(tp));
  auto pr(execute_prerequisites<proto>(a, t, mt));

  bool update(!pr.first);
  target_state ts(update ? target_state::changed : *pr.first);

  const proto &s(pr.second);

  depdb dd(tp + ".d");
  {
    if (dd.expect("proto.compile 1") != nullptr)
      l4([&] { trace << "rule mismatch forcing update of " << t; });

    if (dd.expect(checksum) != nullptr)
      l4([&] { trace << "compiler mismatch forcing update of " << t; });

    sha256 cs;
    append_options(cs, t, "proto.options");

    if (dd.expect(cs.string()) != nullptr)
      l4([&] { trace << "options mismatch forcing update of " << t; });

    if (dd.expect(s.path()) != nullptr)
      l4([&] { trace << "input file mismatch forcing update of " << t; });
  }

  if (dd.writing() || dd.mtime > mt)
    update = true;

  dd.close();

  if (!update)
    return ts;

  path relo(relative(t.dir));
  path rels(relative(s.path()));

  const process_path &pp(this->target.process_path());
  cstrings args{pp.recall_string()};

  args.push_back("--cpp_out");
  args.push_back(relo.string().c_str());

  append_options(args, t, "proto.options");

  args.push_back(rels.string().c_str());
  args.push_back(nullptr);

  if (verb >= 2)
    print_process(args);
  else if (verb)
    text << "protoc " << s;

  if (!t.ctx.dry_run) {
    run(pp, args);
    dd.check_mtime(tp);
  }

  t.mtime(system_clock::now());
  return target_state::changed;
}

} // namespace build2::protobuf
