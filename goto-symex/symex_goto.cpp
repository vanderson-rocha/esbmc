/*******************************************************************\

   Module: Symbolic Execution

   Author: Daniel Kroening, kroening@kroening.com Lucas Cordeiro,
     lcc08r@ecs.soton.ac.uk

\*******************************************************************/

#include <irep2.h>
#include <migrate.h>
#include <assert.h>

#include <expr_util.h>
#include <std_expr.h>

#include "goto_symex.h"

void
goto_symext::symex_goto(const expr2tc &old_guard)
{
  const goto_programt::instructiont &instruction = *cur_state->source.pc;

  expr2tc new_guard = old_guard;
  cur_state->rename(new_guard);
  do_simplify(new_guard);

  if ((is_false(new_guard)) || cur_state->guard.is_false()) {

    // reset unwinding counter
    cur_state->unwind_map[cur_state->source] = 0;

    // next instruction
    cur_state->source.pc++;

    return; // nothing to do
  }

  assert(!instruction.targets.empty());

  // we only do deterministic gotos for now
  if (instruction.targets.size() != 1)
    throw "no support for non-deterministic gotos";

  goto_programt::const_targett goto_target =
    instruction.targets.front();

  bool forward =
    cur_state->source.pc->location_number <
    goto_target->location_number;

  if (!forward) { // backwards?
    unsigned unwind;

    unwind = cur_state->unwind_map[cur_state->source];
    unwind++;
    cur_state->unwind_map[cur_state->source] = unwind;

    if (get_unwind(cur_state->source, unwind)) {
      loop_bound_exceeded(new_guard);

      // reset unwinding
      cur_state->unwind_map[cur_state->source] = 0;

      // next instruction
      cur_state->source.pc++;
      return;
    }

    if (is_true(new_guard)) {
      cur_state->source.pc = goto_target;
      return; // nothing else to do
    }
  }

  goto_programt::const_targett new_state_pc, state_pc;

  if (forward) {
    new_state_pc = goto_target; // goto target instruction
    state_pc = cur_state->source.pc;
    state_pc++; // next instruction
  } else   {
    new_state_pc = cur_state->source.pc;
    new_state_pc++;
    state_pc = goto_target;
  }

  cur_state->source.pc = state_pc;

  // put into state-queue
  statet::goto_state_listt &goto_state_list =
    cur_state->top().goto_state_map[new_state_pc];

  goto_state_list.push_back(statet::goto_statet(*cur_state));
  statet::goto_statet &new_state = goto_state_list.back();

  // adjust guards
  if (is_true(new_guard)) {
    cur_state->guard.make_false();
  } else   {
    // produce new guard symbol
    expr2tc guard_expr;

    if (is_symbol2t(new_guard) ||
        (is_not2t(new_guard) && is_symbol2t(to_not2t(new_guard).value))) {
      guard_expr = new_guard;
    } else {
      guard_expr =
        expr2tc(new symbol2t(type_pool.get_bool(), guard_identifier()));

      expr2tc new_rhs = new_guard;
      new_rhs = expr2tc(new not2t(new_rhs));
      do_simplify(new_rhs);

      cur_state->assignment(guard_expr, new_rhs, false);

      guardt guard;

      target->assignment(
        guard.as_expr(),
        guard_expr, guard_expr,
        new_rhs,
        cur_state->source,
        cur_state->gen_stack_trace(),
        symex_targett::HIDDEN);

      guard_expr = expr2tc(new not2t(guard_expr));
    }

    expr2tc not_guard_expr = expr2tc(new not2t(guard_expr));
    do_simplify(not_guard_expr);

    if (forward) {
      new_state.guard.add(guard_expr);
      cur_state->guard.add(not_guard_expr);
    } else   {
      cur_state->guard.add(guard_expr);
      new_state.guard.add(not_guard_expr);
    }
  }
}

void
goto_symext::merge_gotos(void)
{
  statet::framet &frame = cur_state->top();

  // first, see if this is a target at all
  statet::goto_state_mapt::iterator state_map_it =
    frame.goto_state_map.find(cur_state->source.pc);

  if (state_map_it == frame.goto_state_map.end())
    return;  // nothing to do

  // we need to merge
  statet::goto_state_listt &state_list = state_map_it->second;

  for (statet::goto_state_listt::reverse_iterator
       list_it = state_list.rbegin();
       list_it != state_list.rend();
       list_it++)
  {
    statet::goto_statet &goto_state = *list_it;

    // do SSA phi functions
    phi_function(goto_state);

    merge_value_sets(goto_state);

    // adjust guard
    cur_state->guard |= goto_state.guard;

    // adjust depth
    cur_state->depth = std::min(cur_state->depth, goto_state.depth);
  }

  // clean up to save some memory
  frame.goto_state_map.erase(state_map_it);
}

void
goto_symext::merge_value_sets(const statet::goto_statet &src)
{
  if (cur_state->guard.is_false()) {
    cur_state->value_set = src.value_set;
    return;
  }

  cur_state->value_set.make_union(src.value_set);
}

void
goto_symext::phi_function(const statet::goto_statet &goto_state)
{
  // go over all variables to see what changed
  std::set<expr2tc> variables;

  goto_state.level2.get_variables(variables);
  cur_state->level2.get_variables(variables);

  irep_idt tmp_guard = guard_identifier();

  for (std::set<expr2tc>::const_iterator
       it = variables.begin();
       it != variables.end();
       it++)
  {
    if (goto_state.level2.current_number(*it) ==
        cur_state->level2.current_number(*it))
      continue;  // not changed

    if (to_symbol2t(*it).thename == tmp_guard)
      continue;  // just a guard

    expr2tc orig_name = *it;

    cur_state->get_original_name(orig_name);
    std::string original_identifier = to_symbol2t(orig_name).get_symbol_name();

    try
    {
      // changed!
      const symbolt &symbol = ns.lookup(original_identifier);

      type2tc type;
      typet old_type = symbol.type;
      migrate_type(symbol.type, type);

      expr2tc rhs;

      if (cur_state->guard.is_false()) {
        rhs = expr2tc(new symbol2t(type, symbol.name));
        cur_state->current_name(goto_state, rhs);
      } else if (goto_state.guard.is_false())    {
        rhs = expr2tc(new symbol2t(type, symbol.name));
        cur_state->current_name(goto_state, rhs);
      } else   {
	guardt tmp_guard(goto_state.guard);

	// this gets the diff between the guards
	tmp_guard -= cur_state->guard;

	expr2tc true_val = expr2tc(new symbol2t(type, symbol.name));
	expr2tc false_val = expr2tc(new symbol2t(type, symbol.name));
        cur_state->current_name(goto_state, true_val);
        cur_state->current_name(false_val);
        rhs = expr2tc(new if2t(type, tmp_guard.as_expr(), true_val, false_val));
      }

      exprt tmp_lhs(symbol_expr(symbol));
      expr2tc lhs;
      migrate_expr(tmp_lhs, lhs);
      expr2tc new_lhs = lhs;

      cur_state->assignment(new_lhs, rhs, false);

      guardt true_guard;

      target->assignment(
        true_guard.as_expr(),
        new_lhs, lhs,
        rhs,
        cur_state->source,
        cur_state->gen_stack_trace(),
        symex_targett::HIDDEN);
    }
    catch (const std::string e)
    {
      continue;
    }
  }
}

void
goto_symext::loop_bound_exceeded(const expr2tc &guard)
{
  const irep_idt &loop_id = cur_state->source.pc->location.loopid();

  expr2tc negated_cond;

  if (is_true(guard)) {
    negated_cond = false_expr;
  } else {
    negated_cond = expr2tc(new not2t(guard));
  }

  bool unwinding_assertions =
    !options.get_bool_option("no-unwinding-assertions");

  bool partial_loops =
    options.get_bool_option("partial-loops");

  bool base_case=
    options.get_bool_option("base-case");

  bool forward_condition=
    options.get_bool_option("forward-condition");

  if (base_case)
  {
    // generate unwinding assumption
    expr2tc guarded_expr=negated_cond;
    cur_state->guard.guard_expr(guarded_expr);
    target->assumption(cur_state->guard.as_expr(), guarded_expr, cur_state->source);

    // add to state guard to prevent further assignments
    cur_state->guard.add(negated_cond);
  }
  else if (forward_condition)
  {
    // generate unwinding assertion
    claim(negated_cond,
          "unwinding assertion loop "+id2string(loop_id));

    // add to state guard to prevent further assignments
    cur_state->guard.add(negated_cond);
  }
  else if(!partial_loops)
  {
    if(unwinding_assertions)
    {
      // generate unwinding assertion
      claim(negated_cond, "unwinding assertion loop " + id2string(loop_id));
    } else   {
      // generate unwinding assumption, unless we permit partial loops
      expr2tc guarded_expr = negated_cond;
      cur_state->guard.guard_expr(guarded_expr);
      target->assumption(cur_state->guard.as_expr(), guarded_expr,
                         cur_state->source);
    }

    // add to state guard to prevent further assignments
    cur_state->guard.add(negated_cond);
  }
}

bool
goto_symext::get_unwind(
  const symex_targett::sourcet &source, unsigned unwind)
{
  unsigned id = source.pc->loop_number;
  unsigned long this_loop_max_unwind = max_unwind;

  if (unwind_set.count(id) != 0)
    this_loop_max_unwind = unwind_set[id];

  #if 1
  {
    std::string msg =
      "Unwinding loop " + i2string(id) + " iteration " + i2string(unwind) +
      " " + source.pc->location.as_string();
    std::cout << msg << std::endl;
  }
  #endif

  return this_loop_max_unwind != 0 &&
         unwind >= this_loop_max_unwind;
}
