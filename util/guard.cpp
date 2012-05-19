/*******************************************************************\

Module: Symbolic Execution

Author: Daniel Kroening, kroening@kroening.com

\*******************************************************************/

#include "std_expr.h"

#include "guard.h"

/*******************************************************************\

Function: guardt::as_expr

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

exprt guardt::as_expr(guard_listt::const_iterator it) const
{
  if(it==guard_list.end())
    return true_exprt();
  else if(it==--guard_list.end())
    return migrate_expr_back(guard_list.back());

  exprt dest;
  dest=exprt("and", typet("bool"));
  dest.reserve_operands(guard_list.size());
  for(; it!=guard_list.end(); it++)
  {
    if (!is_bool_type((*it)->type)) {
      std::cerr << "guard is expected to be Boolean" << std::endl;
      abort();
    }

    dest.copy_to_operands(migrate_expr_back(*it));
  }

  return dest;
}

/*******************************************************************\

Function: guardt::add

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void guardt::add(const exprt &expr)
{
  if(expr.is_and() && expr.type().is_bool())
  {
    forall_operands(it, expr)
      add(*it);

    return;
  }

  if(expr.is_true())
  {
  }
  else
  {
    expr2tc tmp;
    migrate_expr(expr, tmp);
    guard_list.push_back(tmp);
  }
}

void guardt::add(const expr2tc &expr)
{
  if (is_and2t(expr))
  {
    const and2t &theand = to_and2t(expr);
    add(theand.side_1);
    add(theand.side_2);
    return;
  }

  if (is_constant_bool2t(expr) && to_constant_bool2t(expr).constant_value)
  {
  }
  else
  {
    guard_list.push_back(expr);
  }
}

/*******************************************************************\

Function: guardt::move

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

void guardt::move(exprt &expr)
{
  if(expr.is_true())
  {
  }
  else
  {
    expr2tc tmp;
    migrate_expr(expr, tmp);
    guard_list.push_back(tmp);
  }
}

/*******************************************************************\

Function: operator -=

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

guardt &operator -= (guardt &g1, const guardt &g2)
{
  guardt::guard_listt::const_iterator it2=g2.guard_list.begin();
  
  while(!g1.guard_list.empty() &&
        it2!=g2.guard_list.end() &&
        g1.guard_list.front()==*it2)
  {
    g1.guard_list.pop_front();
    it2++;
  }

  return g1;
}

/*******************************************************************\

Function: operator |=

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

guardt &operator |= (guardt &g1, const guardt &g2)
{
  if(g2.is_false()) return g1;
  if(g1.is_false()) { g1.guard_list=g2.guard_list; return g1; }

  // find common prefix  
  guardt::guard_listt::iterator it1=g1.guard_list.begin();
  guardt::guard_listt::const_iterator it2=g2.guard_list.begin();
  
  while(it1!=g1.guard_list.end())
  {
    if(it2==g2.guard_list.end())
      break;
      
    if(*it1!=*it2)
      break;

    it1++;
    it2++;
  }
  
  if(it2==g2.guard_list.end()) return g1;

  // end of common prefix
  exprt and_expr1, and_expr2;
  and_expr1=g1.as_expr(it1);
  and_expr2=g2.as_expr(it2);
  
  g1.guard_list.erase(it1, g1.guard_list.end());
  
  exprt tmp(and_expr2);
  tmp.make_not();
  
  if(tmp!=and_expr1)
  {
    if(and_expr1.is_true() || and_expr2.is_true())
    {
    }
    else
    {
      exprt or_expr("or", typet("bool"));
      or_expr.move_to_operands(and_expr1, and_expr2);
      g1.move(or_expr);
    }
  }
  
  return g1;
}

/*******************************************************************\

Function: operator <<

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

std::ostream &operator << (std::ostream &out, const guardt &g)
{
  for (std::list<expr2tc>::const_iterator it = g.guard_list.begin();
       it != g.guard_list.end(); it++)
    out << "*** " << (*it)->pretty() << std::endl;

  return out;
}

/*******************************************************************\

Function: guardt::is_false

  Inputs:

 Outputs:

 Purpose:

\*******************************************************************/

bool guardt::is_false() const
{
  forall_guard(it, guard_list)
    if (is_constant_bool2t(*it) && !to_constant_bool2t(*it).constant_value)
      return true;
      
  return false;
}

void
guardt::dump(void) const
{
  std::cout << *this;
  return;
}
