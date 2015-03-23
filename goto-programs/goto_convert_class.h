/*******************************************************************\

Module: Program Transformation

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#ifndef CPROVER_GOTO_PROGRAMS_GOTO_CONVERT_CLASS_H
#define CPROVER_GOTO_PROGRAMS_GOTO_CONVERT_CLASS_H

#include <list>
#include <queue>
#include <stack>
#include <namespace.h>
#include <guard.h>
#include <std_code.h>
#include <options.h>
#include <message_stream.h>

#include <expr_util.h>

#include "goto_program.h"

class goto_convertt:public message_streamt
{
public:
  void goto_convert(const codet &code, goto_programt &dest);

  goto_convertt(
    contextt &_context,
    const optionst &_options,
    message_handlert &_message_handler):
    message_streamt(_message_handler),
    context(_context),
    options(_options),
    ns(_context),
    temporary_counter(0),
    tmp_symbol_prefix("goto_convertt::"),
    current_block(NULL),
    inductive_step(options.get_bool_option("inductive-step")),
    base_case(options.get_bool_option("base-case")),
    forward_condition(options.get_bool_option("forward-condition")),
    assume_all_states(options.get_bool_option("assume-all-states")),
    disable_inductive_step(true),
    total_states(1)
  {
  }

  virtual ~goto_convertt()
  {
  }

protected:
  contextt &context;
  const optionst &options;
  namespacet ns;
  unsigned temporary_counter;
  std::string tmp_symbol_prefix;

  void goto_convert_rec(const codet &code, goto_programt &dest);

  //
  // tools for symbols
  //
  void new_name(symbolt &symbol);
  const symbolt &lookup(const irep_idt &identifier) const;

  symbolt &new_tmp_symbol(const typet &type);
  symbolt &new_cftest_symbol(const typet &type);

  typedef std::list<irep_idt> tmp_symbolst;
  tmp_symbolst tmp_symbols;

  //
  // side effect removal
  //
  void make_temp_symbol(exprt &expr,goto_programt &dest);
  void read(exprt &expr, goto_programt &dest);
  unsigned int get_expr_number_globals(const exprt & expr);
  unsigned int get_expr_number_globals(const expr2tc & expr);
  void break_globals2assignments(exprt & rhs, goto_programt & dest,const locationt & location);
  void break_globals2assignments(int & atomic, exprt & lhs, exprt & rhs, goto_programt & dest, const locationt & location);
  void break_globals2assignments(int & atomic, exprt & rhs, goto_programt & dest,const locationt & location);
  void break_globals2assignments_rec(exprt & rhs, exprt & atomic_dest, goto_programt & dest,int atomic, const locationt & location);

  // this produces if(guard) dest;
  void guard_program(
    const guardt &guard,
    goto_programt &dest);

  void remove_sideeffects(exprt &expr, guardt &guard,
                          goto_programt &dest,
                          bool result_is_used=true);

  void remove_sideeffects(exprt &expr, goto_programt &dest,
                          bool result_is_used=true);

  void address_of_replace_objects(exprt &expr,
		  	  	  	  	  	  	  goto_programt &dest);

  bool has_sideeffect(const exprt &expr);
  bool has_function_call(const exprt &expr);

  void remove_assignment(exprt &expr, guardt &guard, goto_programt &dest);
  void remove_pre(exprt &expr, guardt &guard, goto_programt &dest);
  void remove_post(exprt &expr, guardt &guard, goto_programt &dest, bool result_is_used);
  void remove_function_call(exprt &expr, guardt &guard, goto_programt &dest, bool result_is_used);
  void remove_cpp_new(exprt &expr, guardt &guard, goto_programt &dest, bool result_is_used);
  void remove_temporary_object(exprt &expr, guardt &guard, goto_programt &dest, bool result_is_used);
  void remove_statement_expression(exprt &expr, guardt &guard, goto_programt &dest, bool result_is_used);
  void remove_gcc_conditional_expression(exprt &expr, guardt &guard, goto_programt &dest);

  virtual void do_cpp_new(const exprt &lhs, const exprt &rhs, goto_programt &dest);

  static void replace_new_object(
    const exprt &object,
    exprt &dest);

  void cpp_new_initializer(
    const exprt &lhs, const exprt &rhs, goto_programt &dest);

  //
  // function calls
  //

  virtual void do_function_call(
    const exprt &lhs,
    const exprt &function,
    const exprt::operandst &arguments,
    goto_programt &dest);

  virtual void do_function_call_if(
    const exprt &lhs,
    const exprt &function,
    const exprt::operandst &arguments,
    goto_programt &dest);

  virtual void do_function_call_symbol(
    const exprt &lhs,
    const exprt &function,
    const exprt::operandst &arguments,
    goto_programt &dest);

  virtual void do_function_call_symbol(const symbolt &symbol __attribute__((unused)))
  {
  }

  virtual void do_function_call_dereference(
    const exprt &lhs,
    const exprt &function,
    const exprt::operandst &arguments,
    goto_programt &dest);

  //
  // conversion
  //
  void convert_sideeffect(exprt &expr, goto_programt &dest);
  void convert_block(const codet &code, goto_programt &dest);
  void convert_decl(const codet &code, goto_programt &dest);
  void convert_expression(const codet &code, goto_programt &dest);
  void convert_assign(const code_assignt &code, goto_programt &dest);
  void convert_cpp_delete(const codet &code, goto_programt &dest);
  void convert_for(const codet &code, goto_programt &dest);
  void convert_while(const codet &code, goto_programt &dest);
  void convert_dowhile(const codet &code, goto_programt &dest);
  void convert_assume(const codet &code, goto_programt &dest);
  void convert_assert(const codet &code, goto_programt &dest);
  void convert_switch(const codet &code, goto_programt &dest);
  void convert_break(const code_breakt &code, goto_programt &dest);
  void convert_return(const code_returnt &code, goto_programt &dest);
  void convert_continue(const code_continuet &code, goto_programt &dest);
  void convert_ifthenelse(const codet &code, goto_programt &dest);
  void convert_init(const codet &code, goto_programt &dest);
  void convert_goto(const codet &code, goto_programt &dest);
  void convert_skip(const codet &code, goto_programt &dest);
  void convert_non_deterministic_goto(const codet &code, goto_programt &dest);
  void convert_label(const code_labelt &code, goto_programt &dest);
  void convert_function_call(const code_function_callt &code, goto_programt &dest);
  void convert_atomic_begin(const codet &code, goto_programt &dest);
  void convert_atomic_end(const codet &code, goto_programt &dest);
  void convert(const codet &code, goto_programt &dest);
  void copy(const codet &code, goto_program_instruction_typet type, goto_programt &dest);

  //
  // Try-catch conversion
  //
  void convert_catch(const codet &code,goto_programt &dest);
  void convert_throw_decl(const exprt &expr, goto_programt &dest);
  void convert_throw_decl_end(const exprt &expr, goto_programt &dest);

  //
  // k-induction conversion
  //
  void add_global_variable_to_state();
  void make_nondet_assign(goto_programt &dest);
  void init_k_indice(goto_programt &dest);
  void assign_current_state(goto_programt &dest);
  void assume_cond(const exprt &cond, const bool &neg, goto_programt &dest);
  void replace_ifthenelse(exprt &expr);
  void get_cs_member(exprt &expr, exprt &result, const typet &type, bool &found);
  bool is_expr_in_state(const exprt &expr);
  void get_struct_components(const exprt &exp, bool is_global = false);
  void check_loop_cond(exprt &cond);
  void assert_cond(const exprt &cond, const bool &neg, goto_programt &dest);
  bool check_expr_const(const exprt &expr);
  void assume_state_vector(array_typet state_vector, goto_programt &dest);
  void assume_all_state_vector(array_typet state_vector, goto_programt &dest);
  void update_state_vector(array_typet state_vector, goto_programt &dest);
  void print_msg(const exprt &tmp);
  void disable_k_induction(void);
  void print_msg_mem_alloc(void);

  inline bool is_inductive_step_active();

  typedef std::set<exprt> loop_varst;

  class loop_block
  {
    public:
      loop_block(unsigned int _state_counter, loop_varst _global_vars)
    : loop_vars(_global_vars),
      _break(false),
      _active(false),
      _state_counter(_state_counter),
      _state(struct_typet())
    {
        // Check if there are any variables on the set already (static and globals)
        // and add them to the state
        if (!loop_vars.size())
          return;

        for (exprt exp : loop_vars)
        {
          unsigned int size = _state.components().size();
          _state.components().resize(size+1);
          _state.components()[size] = (struct_typet::componentt &) exp;
          _state.components()[size].set_name(exp.get_string("identifier"));
          _state.components()[size].pretty_name(exp.get_string("identifier"));
        }
    }

    bool is_active() const;
    void set_active(bool active);

    bool has_break() const;
    void set_break(bool _break);

    struct_typet& get_state();
    void set_state(const struct_typet& state);

    unsigned int get_state_counter() const;
    void set_state_counter(unsigned int state_counter);

    loop_varst loop_vars;

  private:
    bool _break, _active;
    unsigned int _state_counter;
    struct_typet _state;
  };

  typedef std::stack<loop_block*> loop_stackt;
  loop_stackt loop_stack;

  loop_block* current_block;
  loop_varst global_vars;

  void push_new_loop_block();
  void pop_loop_block();

  //
  // gotos
  //

  void finish_gotos();

  typedef std::map<irep_idt, goto_programt::targett> labelst;
  typedef std::set<goto_programt::targett> gotost;
  typedef exprt::operandst caset;
  typedef std::map<goto_programt::targett, caset> casest;

  struct break_continue_targetst
  {
    break_continue_targetst():break_set(false), continue_set(false)
    {
    }

    goto_programt::targett break_target;
    bool break_set;

    goto_programt::targett continue_target;
    bool continue_set;

    void restore(const break_continue_targetst &targets)
    {
      *this=targets;
    }

    void set_break(goto_programt::targett _break_target)
    {
      break_set=true;
      break_target=_break_target;
    }

    void set_continue(goto_programt::targett _continue_target)
    {
      continue_set=true;
      continue_target=_continue_target;
    }
  };

  struct break_continue_switch_targetst:public break_continue_targetst
  {
    break_continue_switch_targetst():
      default_set(false)
    {
    }

    using break_continue_targetst::restore;

    void restore(const break_continue_switch_targetst &targets)
    {
      *this=targets;
    }

    void set_default(goto_programt::targett _default_target)
    {
      default_set=true;
      default_target=_default_target;
    }

    goto_programt::targett default_target;
    bool default_set;
    casest cases;
  };

  struct targetst:public break_continue_switch_targetst
  {
    bool return_set;
    bool return_value;

    labelst labels;
    gotost gotos;

    targetst():
      return_set(false)
    {
    }

    void swap(targetst &targets)
    {
      std::swap(targets.break_target, break_target);
      std::swap(targets.break_set, break_set);

      std::swap(targets.continue_target, continue_target);
      std::swap(targets.continue_set, continue_set);

      std::swap(targets.return_value, return_value);
      std::swap(targets.return_set, return_set);

      std::swap(targets.default_target, default_target);
      std::swap(targets.default_set, default_set);

      targets.labels.swap(labels);
      targets.gotos.swap(gotos);
      targets.cases.swap(cases);
    }
  } targets;

  void case_guard(
    const exprt &value,
    const caset &case_op,
    exprt &dest);

  // if(cond) { true_case } else { false_case }
  void generate_ifthenelse(
    const exprt &cond,
    goto_programt &true_case,
    goto_programt &false_case,
    const locationt &location,
    goto_programt &dest);

  // if(guard) goto target_true; else goto target_false;
  void generate_conditional_branch(
    const exprt &guard,
    goto_programt::targett target_true,
    goto_programt::targett target_false,
    const locationt &location,
    goto_programt &dest);

  // if(guard) goto target;
  void generate_conditional_branch(
    const exprt &guard,
    goto_programt::targett target_true,
    const locationt &location,
    goto_programt &dest);

  // turn a OP b OP c into a list a, b, c
  static void collect_operands(
    const exprt &expr,
    const irep_idt &id,
    std::list<exprt> &dest);

  //
  // misc
  //
  const std::string &get_string_constant(const exprt &expr);

  // some built-in functions
  void do_abort         (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);
  void do_abs           (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);
  void do_atomic_begin  (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);
  void do_atomic_end    (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);
  void do_create_thread (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);
  void do_malloc        (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);
  void do_free          (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);
  void do_sync          (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);
  void do_exit          (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);
  void do_array_set     (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);
  void do_printf        (const exprt &lhs, const exprt &rhs, const exprt::operandst &arguments, goto_programt &dest);

  protected:
    bool inductive_step, base_case, forward_condition, assume_all_states;
    bool disable_inductive_step;
    unsigned int total_states;
};

#endif
