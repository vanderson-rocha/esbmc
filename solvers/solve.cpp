#include "solve.h"

#include <solvers/smtlib/smtlib_conv.h>

#include <solvers/smt/smt_tuple.h>
#include <solvers/smt/smt_array.h>

// For the purpose of vastly reducing build times:
smt_convt *
create_new_minisat_solver(bool int_encoding, const namespacet &ns, bool is_cpp,
                          const optionst &opts);
smt_convt *
create_new_mathsat_solver(bool int_encoding, const namespacet &ns, bool is_cpp,
                          const optionst &opts,
                          tuple_iface **tuple_api, array_iface **array_api);
smt_convt *
create_new_cvc_solver(bool int_encoding, const namespacet &ns, bool is_cpp,
                          const optionst &opts,
                          tuple_iface **tuple_api, array_iface **array_api);
smt_convt *
create_new_boolector_solver(bool int_encoding, const namespacet &ns,
                          bool is_cpp, const optionst &options,
                          tuple_iface **tuple_api, array_iface **array_api);
smt_convt *
create_new_z3_solver(bool int_encoding, const namespacet &ns,
                          bool is_cpp, const optionst &options,
                          tuple_iface **tuple_api, array_iface **array_api);

static smt_convt *
create_z3_solver(bool is_cpp __attribute__((unused)),
                 bool int_encoding __attribute__((unused)),
                 const namespacet &ns __attribute__((unused)),
                 const optionst &opts __attribute__((unused)),
                 tuple_iface **tuple_api __attribute__((unused)),
                 array_iface **array_api __attribute__((unused)))
{
#ifndef Z3
    std::cerr << "Sorry, Z3 support was not built into this version of ESBMC"
              << std::endl;
    abort();
#else
    return create_new_z3_solver(int_encoding, ns, is_cpp, opts, tuple_api, array_api);
#endif
}

static smt_convt *
create_minisat_solver(bool int_encoding __attribute__((unused)),
                      const namespacet &ns __attribute__((unused)),
                      bool is_cpp __attribute__((unused)),
                      const optionst &options __attribute__((unused)))
{
#ifndef MINISAT
    std::cerr << "Sorry, MiniSAT support was not built into this version of "
              "ESBMC" << std::endl;
    abort();
#else
    return create_new_minisat_solver(int_encoding, ns, is_cpp, options);
#endif
}

static smt_convt *
create_mathsat_solver(bool is_cpp __attribute__((unused)),
                                bool int_encoding __attribute__((unused)),
                                const namespacet &ns __attribute__((unused)),
                                const optionst &opts __attribute__((unused)),
                 tuple_iface **tuple_api __attribute__((unused)),
                 array_iface **array_api __attribute__((unused)))
{
#if !defined(MATHSAT)
    std::cerr << "Sorry, MathSAT support was not built into this "
              << "version of ESBMC" << std::endl;
    abort();
#else
    return create_new_mathsat_solver(int_encoding, ns, is_cpp, opts, tuple_api, array_api);
#endif
}

static smt_convt *
create_cvc_solver(bool is_cpp __attribute__((unused)),
                                bool int_encoding __attribute__((unused)),
                                const namespacet &ns __attribute__((unused)),
                                const optionst &opts __attribute__((unused)),
                 tuple_iface **tuple_api __attribute__((unused)),
                 array_iface **array_api __attribute__((unused)))
{
#if !defined(USECVC)
    std::cerr << "Sorry, CVC support was not built into this "
              << "version of ESBMC" << std::endl;
    abort();
#else
    return create_new_cvc_solver(int_encoding, ns, is_cpp, opts, tuple_api, array_api);
#endif
}

static smt_convt *
create_boolector_solver(bool is_cpp __attribute__((unused)),
                                bool int_encoding __attribute__((unused)),
                                const namespacet &ns __attribute__((unused)),
                                const optionst &options __attribute__((unused)),
                 tuple_iface **tuple_api __attribute__((unused)),
                 array_iface **array_api __attribute__((unused)))
{
#if !defined(BOOLECTOR)
    std::cerr << "Sorry, Boolector support was not built into this "
              << "version of ESBMC" << std::endl;
    abort();
#else
    return create_new_boolector_solver(int_encoding, ns, is_cpp, options, tuple_api, array_api);
#endif
}

static const unsigned int num_of_solvers = 8;
static const std::string list_of_solvers[] =
{ "z3", "smtlib", "minisat", "boolector", "sword", "stp", "mathsat", "cvc"};

static smt_convt *
pick_solver(bool is_cpp, bool int_encoding, const namespacet &ns,
            const optionst &options, tuple_iface **tuple_api,
            array_iface **array_api)
{
  unsigned int i, total_solvers = 0;
  for (i = 0; i < num_of_solvers; i++)
    total_solvers += (options.get_bool_option(list_of_solvers[i])) ? 1 : 0;

  *tuple_api = NULL;
  *array_api = NULL;

  if (total_solvers == 0) {
    std::cerr << "No solver specified; defaulting to Z3" << std::endl;
  } else if (total_solvers > 1) {
    std::cerr << "Please only specify one solver" << std::endl;
    abort();
  }

  if (options.get_bool_option("smtlib")) {
    return new smtlib_convt(int_encoding, ns, is_cpp, options);
  } else if (options.get_bool_option("mathsat")) {
    return create_mathsat_solver(int_encoding, is_cpp, ns, options, tuple_api, array_api);
  } else if (options.get_bool_option("cvc")) {
    return create_cvc_solver(int_encoding, is_cpp, ns, options, tuple_api, array_api);
  } else if (options.get_bool_option("minisat")) {
    return create_minisat_solver(int_encoding, ns, is_cpp, options);
  } else if (options.get_bool_option("boolector")) {
    return create_boolector_solver(is_cpp, int_encoding, ns, options, tuple_api, array_api);
  } else {
    return create_z3_solver(is_cpp, int_encoding, ns, options, tuple_api, array_api);
  }
}

smt_convt *
create_solver_factory1(const std::string &solver_name, bool is_cpp,
                       bool int_encoding, const namespacet &ns,
                       const optionst &options,
                       tuple_iface **tuple_api,
                       array_iface **array_api)
{
  if (solver_name == "")
    // Pick one based on options.
    return pick_solver(is_cpp, int_encoding, ns, options, tuple_api, array_api);

  *tuple_api = NULL;
  *array_api = NULL;

  if (solver_name == "z3") {
    return create_z3_solver(is_cpp, int_encoding, ns, options, tuple_api, array_api);
  } else if (solver_name == "mathsat") {
    return create_mathsat_solver(int_encoding, is_cpp, ns, options, tuple_api, array_api);
  } else if (solver_name == "cvc") {
    return create_cvc_solver(int_encoding, is_cpp, ns, options, tuple_api, array_api);
  } else if (solver_name == "smtlib") {
    return new smtlib_convt(int_encoding, ns, is_cpp, options);
  } else if (options.get_bool_option("minisat")) {
    return create_minisat_solver(int_encoding, ns, is_cpp, options);
  } else if (options.get_bool_option("boolector")) {
    return create_boolector_solver(is_cpp, int_encoding, ns, options, tuple_api, array_api);
  } else {
    std::cerr << "Unrecognized solver \"" << solver_name << "\" created"
              << std::endl;
    abort();
  }
}


smt_convt *
create_solver_factory(const std::string &solver_name, bool is_cpp,
                      bool int_encoding, const namespacet &ns,
                      const optionst &options)
{
  tuple_iface *tuple_api = NULL;
  array_iface *array_api = NULL;
  smt_convt *ctx = create_solver_factory1(solver_name, is_cpp, int_encoding, ns, options, &tuple_api, &array_api);

  bool node_flat = options.get_bool_option("tuple-node-flattener");
  bool sym_flat = options.get_bool_option("tuple-sym-flattener");

  // Pick a tuple flattener to use. If the solver has native support, and no
  // options were given, use that by default
  if (tuple_api != NULL && !node_flat && !sym_flat)
    ctx->set_tuple_iface(tuple_api);
  // Use the node flattener if specified
  else if (node_flat)
    ctx->set_tuple_iface(new smt_tuple_node_flattener(ctx, ns));
  // Use the symbol flattener if specified
  else if (sym_flat)
    ctx->set_tuple_iface(new smt_tuple_sym_flattener(ctx, ns));
  // Default: node flattener
  else
    ctx->set_tuple_iface(new smt_tuple_node_flattener(ctx, ns));

  ctx->smt_post_init();
  return ctx;
}
