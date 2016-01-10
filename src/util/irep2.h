#ifndef _UTIL_IREP2_H_
#define _UTIL_IREP2_H_

/** @file irep2.h
 *  Classes and definitions for non-stringy internal representation.
 */

#include <stdarg.h>

#include <vector>

#include <ac_config.h>

#include <boost/mpl/if.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/crc.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/fusion/include/equal_to.hpp>
#include <boost/functional/hash_fwd.hpp>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/equal.hpp>
#include <boost/mpl/not.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/empty.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/pop_front.hpp>

#include <boost/static_assert.hpp>

#include <config.h>
#include <irep.h>
#include <fixedbv.h>
#include <big-int/bigint.hh>
#include <dstring.h>

#include <crypto_hash.h>

// XXXjmorse - abstract, access modifies, need consideration

/** Iterate over all expr2tc's in a vector.
 *  Deals only with constant vectors.
 *  @see Forall_exprs
 *  @param it Name to give iterator to be declared
 *  @param vect Reference to vector of expr2tc's.
 */
#define forall_exprs(it, vect) \
  for (std::vector<expr2tc>::const_iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

/** Iterate over all expr2tc's in a vector.
 *  Deals only with non-constant vectors.
 *  @see forall_exprs
 *  @param it Name to give iterator to be declared
 *  @param vect Reference to vector of expr2tc's.
 */
#define Forall_exprs(it, vect) \
  for (std::vector<expr2tc>::iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

/** Iterate over all type2tc's in a vector.
 *  Deals only with constant vectors
 *  @see Forall_types
 *  @param it Name to give iterator to be declared
 *  @param vect Reference to vector of type2tc's.
 */
#define forall_types(it, vect) \
  for (std::vector<type2tc>::const_iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

/** Iterate over all type2tc's in a vector.
 *  Deals only with non-constant vectors
 *  @see forall_types
 *  @param it Name to give iterator to be declared
 *  @param vect Reference to vector of type2tc's.
 */
#define Forall_types(it, vect) \
  for (std::vector<type2tc>::iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

/** Iterate over all irep_idt's in a vector.
 *  Deals only with constant vectors of string-pool IDs.
 *  @see Forall_names
 *  @param it Name to give iterator to be declared
 *  @param vect Reference to vector of irep_idts's.
 */
#define forall_names(it, vect) \
  for (std::vector<irep_idt>::const_iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

/** Iterate over all irep_idt's in a vector.
 *  Deals only with non-constant vectors of string-pool IDs.
 *  @see forall_names
 *  @param it Name to give iterator to be declared
 *  @param vect Reference to vector of irep_idts's.
 */
#define Forall_names(it, vect) \
  for (std::vector<std::string>::iterator (it) = (vect).begin();\
       it != (vect).end(); it++)

/** Iterate over all expr-like operands in an irep.
 *  This macro automates iterating over sub-expressions in an irep. It takes the
 *  name of an irep container ptr to make, and an integer to track indexes with,
 *  and creates them in scope. A for loop then executes the next statement with
 *  the container ptr pointing at a sub-expr. Fixed-type operands (such as the
 *  member name in a member2t) are not part of the list.
 *
 *  NB: This iterates of expr2tc _pointers_. So, you need to dereference first
 *  the ptr, then the container.
 *
 *  @see Forall_operands2
 *  @param it Name to give iterator to be declared
 *  @param idx Name for index tracking integer.
 *  @param theexpr expr2tc to retrieve list of operands from.
 */
#define forall_operands2(ptr, idx, theexpr) \
  const expr2tc *ptr; \
  unsigned int idx; \
  for (idx = 0, ptr = theexpr->get_sub_expr(0); ptr != 0; \
       idx++, ptr = theexpr->get_sub_expr(idx))

/** Like forall_operands2, but for non-const exprs.
 *
 *  If you feel the need to replace the contents of an expression without
 *  knowing its concrete type (i.e., in simplification) you can assign an
 *  expr2tc into the expr using one of these operand pointers.
 *
 *  Ideally this method should stop existing in the future, and instead we
 *  should constly iterate over sub-exprs, and then call set_sub_expr or
 *  something. I don't think there are that many use cases where every sub expr
 *  gets rewritten, and in these circumstances we're needlessly duplicating
 *  exprs.
 *
 *  @see forall_operands2
 */
#define Forall_operands2(ptr, idx, theexpr) \
  expr2tc *ptr; \
  unsigned int idx; \
  for (idx = 0, ptr = theexpr.get()->get_sub_expr_nc(0); ptr != 0; \
       idx++, ptr = theexpr.get()->get_sub_expr_nc(idx))

// Even crazier forward decs,
namespace esbmct {
  template <typename ...Args> class expr2t_traits;
  typedef expr2t_traits<> expr2t_default_traits;
}

class type2t;
class expr2t;
class constant_array2t;

/** Reference counted container for expr2t based classes.
 *  This class extends boost shared_ptr's to contain anything that's a subclass
 *  of expr2t. It provides several ways of accessing the contained pointer;
 *  crucially it ensures that the only way to get a non-const reference or
 *  pointer is via the get() method, which call the detach() method.
 *
 *  This exists to ensure that we honour the model set forth by the old string
 *  based internal representation - specifically, that if you performed a const
 *  operation on an irept (fetching data) then the contained piece of data
 *  could continue to be shared between numerous data structures, for example
 *  a piece of code could exist in a contextt, a namespacet, and a goto_programt
 *  and all would share the same contained data structure, preventing additional
 *  memory consumption.
 *
 *  If anything copied an irept from one of these place it'd also share that
 *  contained data; but if it made a modifying operation (add, set, or just
 *  taking a non-const reference the contained data,) then the detach() method
 *  would be called, which duplicated the contained item and let the current
 *  piece of code modify the duplicate copy, while all the other storage
 *  locations continued to share the original.
 *
 *  So yeah, that's what this class attempts to implement, via the medium of
 *  boosts shared_ptr.
 */
template <class T>
class irep_container : public std::shared_ptr<T>
{
public:
  irep_container() : std::shared_ptr<T>() {}

  template<class Y>
  explicit irep_container(Y *p) : std::shared_ptr<T>(p)
    { }

  template<class Y>
  explicit irep_container(const Y *p) : std::shared_ptr<T>(const_cast<Y *>(p))
    { }

  irep_container(const irep_container &ref)
    : std::shared_ptr<T>(ref) {}

  template <class Y>
  irep_container(const irep_container<Y> &ref)
    : std::shared_ptr<T>(static_cast<const std::shared_ptr<Y> &>(ref))
  {
    assert(dynamic_cast<const std::shared_ptr<T> &>(ref) != NULL);
  }

  irep_container &operator=(irep_container const &ref)
  {
    std::shared_ptr<T>::operator=(ref);
    return *this;
  }

  template<class Y>
  irep_container & operator=(std::shared_ptr<Y> const & r)
  {
    std::shared_ptr<T>::operator=(r);
    T *p = std::shared_ptr<T>::operator->();
    return *this;
  }

  template <class Y>
  irep_container &operator=(const irep_container<Y> &ref)
  {
    assert(dynamic_cast<const std::shared_ptr<T> &>(ref) != NULL);
    *this = boost::static_pointer_cast<T, Y>
            (static_cast<const std::shared_ptr<Y> &>(ref));
    return *this;
  }

  const T &operator*() const
  {
    return *std::shared_ptr<T>::get();
  }

  const T * operator-> () const // never throws
  {
    return std::shared_ptr<T>::operator->();
  }

  const T * get() const // never throws
  {
    return std::shared_ptr<T>::get();
  }

  T * get() // never throws
  {
    detach();
    T *tmp = std::shared_ptr<T>::get();
    tmp->crc_val = 0;
    return tmp;
  }

  void detach(void)
  {
    if (this->use_count() == 1)
      return; // No point remunging oneself if we're the only user of the ptr.

    // Assign-operate ourself into containing a fresh copy of the data. This
    // creates a new reference counted object, and assigns it to ourself,
    // which causes the existing reference to be decremented.
    const T *foo = std::shared_ptr<T>::get();
    *this = foo->clone();
    return;
  }

  uint32_t crc(void) const
  {
    const T *foo = std::shared_ptr<T>::get();
    if (foo->crc_val != 0)
      return foo->crc_val;

    foo->do_crc(0);
    return foo->crc_val;
  }
};

typedef irep_container<type2t> type2tc;
typedef irep_container<expr2t> expr2tc;

typedef std::pair<std::string,std::string> member_entryt;
typedef std::list<member_entryt> list_of_memberst;

/** Base class for all types.
 *  Contains only a type identifier enumeration - for some types (such as bool,
 *  or empty,) there's no need for any significant amount of data to be stored.
 */
class type2t
{
public:
  /** Enumeration identifying each sort of type. */
  enum type_ids {
    bool_id,
    empty_id,
    symbol_id,
    struct_id,
    union_id,
    code_id,
    array_id,
    pointer_id,
    unsignedbv_id,
    signedbv_id,
    fixedbv_id,
    string_id,
    cpp_name_id,
    end_type_id
  };

  /** Symbolic type exception class.
   *  To be thrown when attempting to fetch the width of a symbolic type, such
   *  as empty or code. Caller will have to worry about what to do about that.
   */
  class symbolic_type_excp {
  };

protected:
  /** Primary constructor.
   *  @param id Type ID of type being constructed
   */
  type2t(type_ids id);

  /** Copy constructor */
  type2t(const type2t &ref);

public:
  // Provide base / container types for some templates stuck on top:
  typedef type2tc container_type;
  typedef type2t base_type;

  virtual ~type2t() { };

  /** Fetch bit width of this type.
   *  For a particular type, calculate its size in a bit representation of
   *  itself. May throw various exceptions depending on whether this operation
   *  is viable - for example, for symbol types, infinite sized or dynamically
   *  sized arrays.
   *
   *  Note that the bit width is _not_ the same as the ansi-c byte model
   *  representation of this type.
   *
   *  @throws symbolic_type_excp
   *  @throws array_type2t::inf_sized_array_excp
   *  @throws array_type2t::dyn_sized_array_excp
   *  @return Size of types byte representation, in bits
   */
  virtual unsigned int get_width(void) const = 0;

  /* These are all self explanatory */
  bool operator==(const type2t &ref) const;
  bool operator!=(const type2t &ref) const;
  bool operator<(const type2t &ref) const;

  /** Produce a string representation of type.
   *  Takes body of the current type and produces a human readable
   *  representation. Similar to the string-irept's pretty method, although a
   *  different format.
   *  @param indent Number of spaces to indent lines by in the output
   *  @return String obj containing representation of this object
   */
  std::string pretty(unsigned int indent = 0) const;

  /** Dump object string representation to stdout.
   *  This take the output of the pretty method, and dumps it to stdout. To be
   *  used for debugging and when single stepping in gdb.
   *  @see pretty
   */
  void dump(void) const;

  /** Produce a checksum/hash of the current object.
   *  Takes current object and produces a lossy digest of it. Originally used
   *  crc32, now uses a more hacky but faster hash function. For use in hash
   *  objects.
   *  @see do_crc
   *  @return Digest of the current type.
   */
  uint32_t crc(void) const;

  /** Perform checked invocation of cmp method.
   *  Takes reference to another type - if they have the same type id, invoke
   *  the cmp function and return its result. Otherwise, return false. Using
   *  this method ensures thatthe implementer of cmp knows the reference it
   *  operates on is on the same type as itself.
   *  @param ref Reference to type to compare this object against
   *  @return True if types are the same, false otherwise.
   */
  bool cmpchecked(const type2t &ref) const;

  /** Perform checked invocation of lt method.
   *  Identical to cmpchecked, except with the lt method.
   *  @see cmpchecked
   *  @param ref Reference to type to measure this against.
   *  @return 0 if types are the same, 1 if this > ref, -1 if ref > this.
   */
  int ltchecked(const type2t &ref) const;

  /** Virtual method to compare two types.
   *  To be overridden by an extending type; assumes that itself and the
   *  parameter are of the same extended type. Call via cmpchecked.
   *  @see cmpchecked
   *  @param ref Reference to (same class of) type to compare against
   *  @return True if types match, false otherwise
   */
  virtual bool cmp(const type2t &ref) const = 0;

  /** Virtual method to compare two types.
   *  To be overridden by an extending type; assumes that itself and the
   *  parameter are of the same extended type. Call via cmpchecked.
   *  @see cmpchecked
   *  @param ref Reference to (same class of) type to compare against
   *  @return 0 if types are the same, 1 if this > ref, -1 if ref > this.
   */
  virtual int lt(const type2t &ref) const;

  /** Extract a list of members from type as strings.
   *  Produces a list of pairs, mapping a member name to a string value. Used
   *  in the body of the pretty method.
   *  @see pretty
   *  @param indent Number of spaces to indent output strings with, if multiline
   *  @return list of name:value pairs.
   */
  virtual list_of_memberst tostring(unsigned int indent) const = 0;

  /** Perform crc operation accumulating into parameter.
   *  Performs the operation of the crc method, but overridden to be specific to
   *  a particular type. Accumulates data into the hash object parameter.
   *  @see cmp
   *  @param seed Hash to accumulate hash data into.
   *  @return Hash value
   */
  virtual size_t do_crc(size_t seed) const;

  /** Perform hash operation accumulating into parameter.
   *  Feeds data as appropriate to the type of the expression into the
   *  parameter, to be hashed. Like crc and do_crc, but for some other kind
   *  of hash scenario.
   *  @see cmp
   *  @see crc
   *  @see do_crc
   *  @param hash Object to accumulate hash data into.
   */
  virtual void hash(crypto_hash &hash) const;

  /** Clone method. Self explanatory.
   *  @return New container, containing a duplicate of this object.
   */
  virtual type2tc clone(void) const = 0;

  /** Instance of type_ids recording this types type. */
  // XXX XXX XXX this should be const
  type_ids type_id;

  mutable size_t crc_val;
};

/** Fetch identifying name for a type.
 *  I.E., this is the class of the type, what you'd get if you called type.id()
 *  with the old stringy irep. Ideally this should be a class method, but as it
 *  was added as a hack I haven't got round to it yet.
 *  @param type Type to fetch identifier for
 *  @return String containing name of type class.
 */
std::string get_type_id(const type2t &type);

/** Fetch identifying name for a type.
 *  Just passes through to type2t accepting function with the same name.
 *  @param type Type to fetch identifier for
 *  @return String containing name of type class.
 */
static inline std::string get_type_id(const type2tc &type)
{
  return get_type_id(*type);
}

/** Base class for all expressions.
 *  In this base, contains an expression id used for distinguishing different
 *  classes of expr, in addition we have a type as all exprs should have types.
 */
class expr2t;
class expr2t : public std::enable_shared_from_this<expr2t>
{
public:
  /** Enumeration identifying each sort of expr.
   */
  enum expr_ids {
    constant_int_id,
    constant_fixedbv_id,
    constant_bool_id,
    constant_string_id,
    constant_struct_id,
    constant_union_id,
    constant_array_id,
    constant_array_of_id,
    symbol_id,
    typecast_id,
    if_id,
    equality_id,
    notequal_id,
    lessthan_id,
    greaterthan_id,
    lessthanequal_id,
    greaterthanequal_id,
    not_id,
    and_id,
    or_id,
    xor_id,
    implies_id,
    bitand_id,
    bitor_id,
    bitxor_id,
    bitnand_id,
    bitnor_id,
    bitnxor_id,
    bitnot_id,
    lshr_id,
    neg_id,
    abs_id,
    add_id,
    sub_id,
    mul_id,
    div_id,
    modulus_id,
    shl_id,
    ashr_id,
    dynamic_object_id, // Not converted in SMT, only in goto-symex
    same_object_id,
    pointer_offset_id,
    pointer_object_id,
    address_of_id,
    byte_extract_id,
    byte_update_id,
    with_id,
    member_id,
    index_id,
    zero_string_id,
    zero_length_string_id,
    isnan_id,
    overflow_id,
    overflow_cast_id,
    overflow_neg_id,
    unknown_id,
    invalid_id,
    null_object_id,
    dereference_id,
    valid_object_id,
    deallocated_obj_id,
    dynamic_size_id,
    sideeffect_id,
    code_block_id,
    code_assign_id,
    code_init_id,
    code_decl_id,
    code_printf_id,
    code_expression_id,
    code_return_id,
    code_skip_id,
    code_free_id,
    code_goto_id,
    object_descriptor_id,
    code_function_call_id,
    code_comma_id,
    invalid_pointer_id,
    buffer_size_id,
    code_asm_id,
    code_cpp_del_array_id,
    code_cpp_delete_id,
    code_cpp_catch_id,
    code_cpp_throw_id,
    code_cpp_throw_decl_id,
    code_cpp_throw_decl_end_id,
    isinf_id,
    isnormal_id,
    concat_id,
    end_expr_id
  };

  /** Type for list of constant expr operands */
  typedef std::list<const expr2tc*> expr_operands;
  /** Type for list of non-constant expr operands */
  typedef std::list<expr2tc*> Expr_operands;

protected:
  /** Primary constructor.
   *  @param type Type of this new expr
   *  @param id Class identifier for this new expr
   */
  expr2t(const type2tc type, expr_ids id);
  /** Copy constructor */
  expr2t(const expr2t &ref);

public:
  // Provide base / container types for some templates stuck on top:
  typedef expr2tc container_type;
  typedef expr2t base_type;
  // Also provide base traits
  typedef esbmct::expr2t_default_traits traits;

  virtual ~expr2t() { };

  /** Clone method. Self explanatory. */
  virtual expr2tc clone(void) const = 0;

  /* These are all self explanatory */
  bool operator==(const expr2t &ref) const;
  bool operator<(const expr2t &ref) const;
  bool operator!=(const expr2t &ref) const;

  /** Perform type-checked call to lt method.
   *  Checks that this object and the one we're comparing against have the same
   *  expr class, so that the lt method can assume it's working on objects of
   *  the same type.
   *  @see type2t::ltchecked
   *  @param ref Expression object we're comparing this object against.
   *  @return 0 If exprs are the same, 1 if this > ref, -1 if ref > this.
   */
  int ltchecked(const expr2t &ref) const;

  /** Produce textual representation of this expr.
   *  Like the stringy-irep's pretty method, this takes the current object and
   *  produces a textual representation that can be read by a human to
   *  understand what's going on.
   *  @param indent Number of spaces to indent the output string lines by
   *  @return String object containing textual expr representation.
   */
  std::string pretty(unsigned int indent = 0) const;

  /** Calculate number of exprs descending from this one.
   *  For statistics collection - calculates the number of expressions that
   *  make up this particular expression (i.e., count however many expr2tc's you
   *  can reach from this expr).
   *  @return Number of expr2tc's reachable from this node.
   */
  unsigned long num_nodes(void) const;

  /** Calculate max depth of exprs from this point.
   *  Looks at all sub-exprs of this expr, and calculates the longest chain one
   *  can descend before there are no more. Useful for statistics about the
   *  exprs we're dealing with.
   *  @return Number of expr2tc's reachable from this node.
   */
  unsigned long depth(void) const;

  /** Write textual representation of this object to stdout.
   *  For use in debugging - dumps the output of the pretty method to stdout.
   *  Can either be used in portion of code, or more commonly called from gdb.
   */
  void dump(void) const;

  /** Calculate a hash/digest of the current expr.
   *  For use in hash data structures; used to be a crc32, but is now a 16 bit
   *  hash function generated by myself to be fast. May not have nice
   *  distribution properties, but is at least fast.
   *  @return Hash value of this expr
   */
  uint32_t crc(void) const;

  /** Perform comparison operation between this and another expr.
   *  Overridden by subclasses of expr2t to compare different members of this
   *  and the passed in object. Assumes that the passed in object is the same
   *  class type as this; Should be called via operator==, which will do that
   *  check automagically.
   *  @see type2t::cmp
   *  @param ref Expr object to compare this against
   *  @return True if objects are the same; false otherwise.
   */
  virtual bool cmp(const expr2t &ref) const;

  /** Compare two expr objects.
   *  Overridden by subclasses - takes two expr objects (this and ref) of the
   *  same type, and compares them, in the same manner as memcmp. The assumption
   *  that the objects are of the same type means lt should be called via
   *  ltchecked to check for different expr types.
   *  @see type2t::lt
   *  @param ref Expr object to compare this against
   *  @return 0 If exprs are the same, 1 if this > ref, -1 if ref > this.
   */
  virtual int lt(const expr2t &ref) const;

  /** Convert fields of subclasses to a string representation.
   *  Used internally by the pretty method - creates a list of pairs
   *  representing the fields in the subclass. Each pair is a pair of strings
   *  of the form fieldname : value. The value may be multiline, in which case
   *  the new line will have at least indent number of indenting spaces.
   *  @param indent Number of spaces to indent multiline output by
   *  @return list of string pairs, of form fieldname:value
   */
  virtual list_of_memberst tostring(unsigned int indent) const = 0;

  /** Perform digest/hash function on expr object.
   *  Takes all fields in this exprs and adds them to the passed in hash object
   *  to compute an expression-hash. Overridden by subclasses.
   *  @param seed Hash to accumulate expression data into.
   *  @return Hash value
   */
  virtual size_t do_crc(size_t seed) const;

  /** Perform hash operation accumulating into parameter.
   *  Feeds data as appropriate to the type of the expression into the
   *  parameter, to be hashed. Like crc and do_crc, but for some other kind
   *  of hash scenario.
   *  @see cmp
   *  @see crc
   *  @see do_crc
   *  @param hash Object to accumulate hash data into.
   */
  virtual void hash(crypto_hash &hash) const;

  /** Generate a list of expr operands.
   *  Use forall_operands2 instead; this method is overridden by subclasses and
   *  when invoked fills the inp list with pointers to any exprs that make up
   *  this expr. Any fields that aren't expr-based (i.e., the field name in
   *  a member2t expr) are not entered into this list.
   *
   *  Comes in a const and non-const flavour. When using the non-const flavour,
   *  one can overwrite field-exprs within another expr without knowing the
   *  exprs concrete type. In this case, it's immensely important to preserve
   *  type correctness.
   *
   *  @see forall_operands2
   *  @see Forall_operands2
   *  @param inp List of pointers to exprs that are in fields of this expr.
   */
  virtual void list_operands(std::list<const expr2tc*> &inp) const = 0;

  /** Generate a list of expr operands.
   *  See the other const version of this method
   */
  virtual void list_operands(std::list<expr2tc*> &inp) = 0;

  /** Fetch a sub-operand.
   *  These can come out of any field that is an expr2tc, or contains them.
   *  No particular numbering order is promised.
   */
  virtual const expr2tc *get_sub_expr(unsigned int idx) const = 0 ;

  /** Fetch a sub-operand. Non-const version.
   *  These can come out of any field that is an expr2tc, or contains them.
   *  No particular numbering order is promised.
   */
  virtual expr2tc *get_sub_expr_nc(unsigned int idx) = 0;

  /** Count the number of sub-exprs there are.
   */
  virtual unsigned int get_num_sub_exprs(void) const = 0 ;

  /** Simplify an expression.
   *  Similar to simplification in the string-based irep, this generates an
   *  expression with any calculations or operations that can be simplified,
   *  simplified. In contrast to the old form though, this creates a new expr
   *  if something gets simplified, just to make it clear exactly what's
   *  going on.
   *  @return Either a nil expr (null pointer contents) if nothing could be
   *          simplified or a simplified expression.
   */
  expr2tc simplify(void) const;

  /** expr-specific simplification methods.
   *  By default, an expression can't be simplified, and this method returns
   *  a nil expression to show that. However if simplification is possible, the
   *  subclass overrides this and if it can simplify its operands, returns a
   *  new simplified expression. It should attempt to modify itself (it's
   *  const).
   *
   *  If simplification failed the first time around, the simplify method will
   *  simplify this expressions operands for it via the medium of list_operands,
   *  and will then call an expr with the simplified operands to see if it's now
   *  become simplifiable. This call occurs whether or not any operands were
   *  actually simplified, see below.
   *
   *  The 'second' parameter can be used to avoid invoking expensive attempts
   *  to simplify an expression more than once - on the first call to
   *  do_simplify this parameter will be false, then on the second it's be true,
   *  allowing method implementation to save the expensive stuff until all of
   *  its operands have certainly been simplified.
   *
   *  Currently simplification does some things that it shouldn't: pointer
   *  arithmetic for example. I'm not sure where this can be relocated to
   *  though.
   *  @param second Whether this is the second call to do_simplify on this obj
   *  @return expr2tc A nil expression if no simplifcation could occur, or a new
   *          simplified object if it can.
   */
  virtual expr2tc do_simplify(bool second = false) const;

  /** Instance of expr_ids recording tihs exprs type. */
  const expr_ids expr_id;

  /** Type of this expr. All exprs have a type. */
  type2tc type;

  mutable size_t crc_val;
};

inline bool is_nil_expr(const expr2tc &exp)
{
  if (exp.get() == NULL)
    return true;
  return false;
}

inline bool is_nil_type(const type2tc &t)
{
  if (t.get() == NULL)
    return true;
  return false;
}

// For boost multi-index hashing,
inline std::size_t
hash_value(const expr2tc &expr)
{
  return expr.crc();
}

/** Fetch string identifier for an expression.
 *  Returns the class name of the expr passed in - this is equivalent to the
 *  result of expr.id() in old stringy irep. Should ideally be a method of
 *  expr2t, but haven't got around to moving it yet.
 *  @param expr Expression to operate upon
 *  @return String containing class name of expression.
 */
std::string get_expr_id(const expr2t &expr);

/** Fetch string identifier for an expression.
 *  Like the expr2t equivalent with the same name, but de-ensapculates an
 *  expr2tc.
 */
static inline std::string get_expr_id(const expr2tc &expr)
{
  return get_expr_id(*expr);
}

// Forward dec for some subsequent constructor hacks.
class symbol2t;

/** A namespace for "ESBMC templates".
 *  This means anything designed to mess with expressions or types declared in
 *  this header, via the medium of templates. */
namespace esbmct {

  class blank_method_operand {
  };

  /** Maximum number of fields to support in expr2t subclasses. This value
   *  controls the types of any arrays that need to consider the number of
   *  fields. Unfortunately it can't control template parameters, because
   *  vardic templates are C++11. */
  const unsigned int num_type_fields = 6;

  // Dummy type tag - exists to be an arbitary, local class, for use in some
  // templates. See below.
  class dummy_type_tag {
  public:
    typedef int type;
  };

  // Syntactic sugar for some type munging, see below.
  #define enable_if_eq(arbitary, c1, c2) typename boost::lazy_enable_if<boost::fusion::result_of::equal_to<c1,c2>, arbitary>::type* = NULL
  #define enable_if_not_eq(arbitary, c1, c2) typename boost::lazy_enable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<c1,c2> >, arbitary>::type* = NULL
  #define disable_if_eq(arbitary, c1, c2) typename boost::lazy_disable_if<boost::fusion::result_of::equal_to<c1,c2>, arbitary>::type* = NULL
  #define disable_if_not_eq(arbitary, c1, c2) typename boost::lazy_disable_if<boost::mpl::not_<boost::fusion::result_of::equal_to<c1,c2> >, arbitary>::type* = NULL

  /** Template for providing templated methods to expr2t classes.
   *
   *  What this does: we give expr_methods a set of template parameters that
   *  describe the structure of an expr2t subclass. For each field, we give
   *  the template:
   *
   *    - The type of the field
   *    - The class that field is part of
   *    - A pointer offset to that field.
   *
   *  What this means, is that we can @a type @a generically access a member
   *  of a class from within the template, without knowing what type it is,
   *  what its name is, or even what type contains it.
   *
   *  We can then use that to make all the boring methods of expr2t type
   *  generic too. For example: we can make the comparision method by accessing
   *  each field in the class we're dealing with, passing them to another
   *  function to do the comparison (with the type resolved by templates or
   *  via overloading), and then inspecting the output of that.
   *
   *  In fact, we can make type generic implementations of all the following
   *  methods in expr2t: clone, tostring, cmp, lt, do_crc, list_operands, hash.
   *
   *  So, that's what this template provides; an expr2t class can be made by
   *  inheriting from this template, telling it what class it'll end up with,
   *  and what to subclass from, and what the fields in the class being derived
   *  from look like. This means we can construct a type hierarchy with whatever
   *  inheretence we like and whatever fields we like, then latch expr_methods
   *  on top of that to implement all the anoying boring boilerplate code.
   *
   *  ----
   *
   *  The constructors also need come documentation - We want to be able to
   *  pass constructor arguments down to the class we're deriving from
   *  without any additional boilerplate. However, unfortunately, we can't
   *  make a vardic constructor, nor generate a bunch of explicit constructors
   *  because they'll attempt to link against subclass constructors that don't
   *  exist.
   *
   *  The solution to this is a series of explicit constructors that get
   *  disabled by some boost magic depending on what the template parameters
   *  are. The upshot is that however many number of template arguments are
   *  provided, a constructor with that many arguments (plus expr id and type)
   *  is enabled. For more information on how this is made possible, first
   *  consult a doctor, then mail jmorse.
   *
   */

  /** Vardic type2t boilerplate methods. */

  template <typename R, typename C, R C::* v>
    class field_traits
  {
  public:
    typedef R result_type;
    typedef C source_class;
    typedef R C::* membr_ptr;
    static constexpr membr_ptr value = v;
  };

  // Field traits specialization for const fields, i.e. expr_id. These become
  // landmines for future mutable methods, i.e. get_sub_expr, which may get
  // it's consts mixed up.
  template <typename R, typename C, R C::* v>
    class field_traits<const R, C, v>
  {
  public:
    typedef R result_type;
    typedef C source_class;
    typedef const R C::* membr_ptr;
    static constexpr membr_ptr value = v;
  };

  template <typename ...Args>
    class type2t_traits
  {
  public:
    typedef field_traits<type2t::type_ids, type2t, &type2t::type_id> type_id_field;
    typedef typename boost::mpl::push_front<boost::mpl::vector<Args...>, type_id_field>::type type;
  };

  typedef type2t_traits<>::type type2t_default_traits;

  template <typename ...Args>
    class expr2t_traits
  {
  public:
    typedef field_traits<const expr2t::expr_ids, expr2t, &expr2t::expr_id> expr_id_field;
    typedef typename boost::mpl::push_front<boost::mpl::vector<Args...>, expr_id_field>::type type;
    static constexpr bool always_construct = false;
  };

  // Hack to force something2tc to always construct the traits' type, rather
  // that copy construct. Due to misery and ambiguity elsewhere.
  template <typename ...Args>
    class expr2t_traits_always_construct
  {
  public:
    typedef field_traits<const expr2t::expr_ids, expr2t, &expr2t::expr_id> expr_id_field;
    typedef typename boost::mpl::push_front<boost::mpl::vector<Args...>, expr_id_field>::type type;
    static constexpr bool always_construct = true;
  };

//  typedef expr2t_traits<> expr2t_default_traits;

  // Declaration
  template <class derived, class baseclass, typename traits, typename enable = void>
    class type_methods2;
  template <class derived, class baseclass, typename traits, typename enable = void>
    class expr_methods2;

  // Recursive instance
  template <class derived, class baseclass, typename traits, typename enable>
    class type_methods2 : public type_methods2<derived, baseclass, typename boost::mpl::pop_front<traits>::type, enable>
  {
  public:
    typedef type_methods2<derived, baseclass, typename boost::mpl::pop_front<traits>::type, enable> superclass;
    typedef typename baseclass::container_type container2tc;
    typedef typename baseclass::base_type base2t;

    template <typename ...Args> type_methods2(Args... args) : superclass(args...) { }

    // Copy constructor. Construct from derived ref rather than just
    // type_methods2, because the template above will be able to directly
    // match a const derived &, and so the compiler won't cast it up to
    // const type_methods2 & and call the copy constructor. Fix this by
    // defining a copy constructor that exactly matches the (only) use case.
    type_methods2(const derived &ref) : superclass(ref) { }

    container2tc clone(void) const;
    list_of_memberst tostring(unsigned int indent) const;
    bool cmp(const base2t &ref) const;
    int lt(const base2t &ref) const;
    size_t do_crc(size_t seed) const;
    void hash(crypto_hash &hash) const;

  protected:
    typedef typename boost::mpl::front<traits>::type::result_type cur_type;
    typedef typename boost::mpl::front<traits>::type::source_class base_class;
    typedef typename boost::mpl::front<traits>::type membr_ptr;

    void tostring_rec(unsigned int idx, list_of_memberst &vec, unsigned int indent) const;
    bool cmp_rec(const base2t &ref) const;
    int lt_rec(const base2t &ref) const;
    void do_crc_rec() const;
    void hash_rec(crypto_hash &hash) const;

    // These methods are specific to expressions rather than types, and are
    // placed here to avoid un-necessary recursion in expr_methods2.
    void list_operands_rec(std::list<const expr2tc*> &inp) const;
    void list_operands_rec(std::list<expr2tc*> &inp);
    const expr2tc *get_sub_expr_rec(unsigned int cur_count, unsigned int desired) const;
    expr2tc *get_sub_expr_nc_rec(unsigned int cur_count, unsigned int desired);
    unsigned int get_num_sub_exprs_rec(void) const;
  };

  // Base instance
  template <class derived, class baseclass, typename X>
    class type_methods2<derived, baseclass, X,
                        typename boost::enable_if<typename boost::mpl::empty<X>::type>::type>
      : public baseclass
  {
  public:
    template <typename ...Args> type_methods2(Args... args) : baseclass(args...) { }

    // Copy constructor. See note for non-specialized definition.
    type_methods2(const derived &ref) : baseclass(ref) { }

  protected:
    typedef typename baseclass::container_type container2tc;
    typedef typename baseclass::base_type base2t;

    // Rather than trying to specialize and implement in the cpp file, terminate
    // here.
    void tostring_rec(unsigned int idx, list_of_memberst &vec, unsigned int indent) const
    {
      (void)idx;
      (void)vec;
      (void)indent;
      return;
    }

    bool cmp_rec(const base2t &ref) const
    {
      // If it made it this far, we passed
      (void)ref;
      return true;
    }

    int lt_rec(const base2t &ref) const
    {
      // If it made it this far, we passed
      (void)ref;
      return 0;
    }

    void do_crc_rec() const
    {
      return;
    }

    void hash_rec(crypto_hash &hash) const
    {
      (void)hash;
      return;
    }

    // Expr methods
    void list_operands_rec(std::list<const expr2tc*> &inp) const
    {
      (void)inp;
      return;
    }

    void list_operands_rec(std::list<expr2tc*> &inp)
    {
      (void)inp;
      return;
    }

    const expr2tc *get_sub_expr_rec(unsigned int cur_idx, unsigned int desired) const
    {
      // No result, so desired must exceed the number of idx's
      assert(cur_idx >= desired);
      return NULL;
    }

    expr2tc *get_sub_expr_nc_rec(unsigned int cur_idx, unsigned int desired)
    {
      // See above
      assert(cur_idx >= desired);
      return NULL;
    }

    unsigned int get_num_sub_exprs_rec(void) const
    {
      return 0;
    }
  };

  // Head definition of expr_methods2 XXX XXX explain
  template <class derived, class baseclass, typename traits, typename enable>
    class expr_methods2 : public type_methods2<derived, baseclass, traits, enable>
  {
  public:
    typedef type_methods2<derived, baseclass, traits, enable> superclass;

    template <typename ...Args> expr_methods2(Args... args) : superclass(args...) { }

    // See notes on type_methods2 copy constructor
    expr_methods2(const derived &ref) : superclass(ref) { }

    void list_operands(std::list<const expr2tc*> &inp) const;
    const expr2tc *get_sub_expr(unsigned int i) const;
    expr2tc *get_sub_expr_nc(unsigned int i);
    unsigned int get_num_sub_exprs(void) const;
  protected:
    void list_operands(std::list<expr2tc*> &inp);
  };

  // Meta goo
  class takestype;
  class notype;

  // So that we can write such things as:
  //
  //   constant_int2tc bees(type, val);
  //
  // We need a class derived from expr2tc that takes the correct set of
  // constructor arguments, which means yet more template goo.
  template <class contained, unsigned int expid, class superclass,class hastype>
  class something2tc : public expr2tc {
    public:
    // Blank initialization of a container class -> store NULL
    something2tc() : expr2tc() { }

    // Initialize container from a non-type-committed container. Encode an
    // assertion that the type is what we expect.
    //
    // Don't do this though if this'll conflict with a later consructor though.
    // For example if we have not2tc, not2tc(expr) could be copying it or
    // constructing a new not2t irep. In the face of this ambiguity, pick the
    // latter, and the end user can worry about how to cast up to a not2tc.
    template <class arbitary = ::esbmct::dummy_type_tag>
    something2tc(const expr2tc &init,
                 typename boost::lazy_disable_if<boost::mpl::bool_<contained::traits::always_construct == true>, arbitary>::type* = NULL
                 ) : expr2tc(init)
    {
      assert(init->expr_id == expid);
    }

    const contained &operator*() const
    {
      return static_cast<const contained&>(*expr2tc::get());
    }

    const contained * operator-> () const // never throws
    {
      return static_cast<const contained*>(expr2tc::operator->());
    }

    const contained * get() const // never throws
    {
      return static_cast<const contained*>(expr2tc::get());
    }

    contained * get() // never throws
    {
      detach();
      return static_cast<contained*>(expr2tc::get());
    }

    // Forward all constructors down to the contained type.
    template <typename ...Args>
    something2tc(Args... args) : expr2tc(new contained(args...)) { }
  };
}; // esbmct

// So - make some type definitions for the different types we're going to be
// working with. This is to avoid the repeated use of template names in later
// definitions. If you'd like to add another type - don't. Vast tracts of code
// only expect the types below, it's be extremely difficult to hack new ones in.

// Start with forward class definitions

class bool_type2t;
class empty_type2t;
class symbol_type2t;
class struct_type2t;
class union_type2t;
class bv_type2t;
class unsignedbv_type2t;
class signedbv_type2t;
class code_type2t;
class array_type2t;
class pointer_type2t;
class fixedbv_type2t;
class string_type2t;
class cpp_name_type2t;

// We also require in advance, the actual classes that store type data.

class symbol_type_data : public type2t
{
public:
  symbol_type_data(type2t::type_ids id, const dstring sym_name) :
    type2t (id), symbol_name(sym_name) {}
  symbol_type_data(const symbol_type_data &ref) :
    type2t (ref), symbol_name(ref.symbol_name) { }

  irep_idt symbol_name;

// Type mangling:
  typedef esbmct::field_traits<irep_idt, symbol_type_data, &symbol_type_data::symbol_name> symbol_name_field;
  typedef esbmct::type2t_traits<symbol_name_field> traits;
};

class struct_union_data : public type2t
{
public:
  struct_union_data(type2t::type_ids id, const std::vector<type2tc> &membs,
                     const std::vector<irep_idt> &names, const irep_idt &n)
    : type2t(id), members(membs), member_names(names), name(n) { }
  struct_union_data(const struct_union_data &ref)
    : type2t(ref), members(ref.members), member_names(ref.member_names),
      name(ref.name) { }

  /** Fetch index number of member. Given a textual name of a member of a
   *  struct or union, this method will look up what index it is into the
   *  vector of types that make up this struct/union. Always returns the correct
   *  index, if you give it a name that isn't part of this struct/union it'll
   *  abort.
   *  @param name Name of member of this struct/union to look up.
   *  @return Index into members/member_names vectors */
  unsigned int get_component_number(const irep_idt &name) const;

  const std::vector<type2tc> & get_structure_members(void) const;
  const std::vector<irep_idt> & get_structure_member_names(void) const;
  const irep_idt & get_structure_name(void) const;

  std::vector<type2tc> members;
  std::vector<irep_idt> member_names;
  irep_idt name;

// Type mangling:
  typedef esbmct::field_traits<std::vector<type2tc>, struct_union_data, &struct_union_data::members> members_field;
  typedef esbmct::field_traits<std::vector<irep_idt>, struct_union_data, &struct_union_data::member_names> member_names_field;
  typedef esbmct::field_traits<irep_idt, struct_union_data, &struct_union_data::name> name_field;
  typedef esbmct::type2t_traits<members_field, member_names_field, name_field> traits;
};

class bv_data : public type2t
{
public:
  bv_data(type2t::type_ids id, unsigned int w) : type2t(id), width(w)
  {
    // assert(w != 0 && "Must have nonzero width for integer type");
    // XXX -- zero sized bitfields are permissible. Oh my.
  }
  bv_data(const bv_data &ref) : type2t(ref), width(ref.width) { }

  virtual unsigned int get_width(void) const;

  unsigned int width;

// Type mangling:
  typedef esbmct::field_traits<unsigned int, bv_data, &bv_data::width> width_field;
  typedef esbmct::type2t_traits<width_field> traits;
};

class code_data : public type2t
{
public:
  code_data(type2t::type_ids id, const std::vector<type2tc> &args,
            const type2tc &ret, const std::vector<irep_idt> &names, bool e)
    : type2t(id), arguments(args), ret_type(ret), argument_names(names),
      ellipsis(e) { }
  code_data(const code_data &ref)
    : type2t(ref), arguments(ref.arguments), ret_type(ref.ret_type),
      argument_names(ref.argument_names), ellipsis(ref.ellipsis) { }

  virtual unsigned int get_width(void) const;

  std::vector<type2tc> arguments;
  type2tc ret_type;
  std::vector<irep_idt> argument_names;
  bool ellipsis;

// Type mangling:
  typedef esbmct::field_traits<std::vector<type2tc>, code_data, &code_data::arguments> arguments_field;
  typedef esbmct::field_traits<type2tc, code_data, &code_data::ret_type> ret_type_field;
  typedef esbmct::field_traits<std::vector<irep_idt>, code_data, &code_data::argument_names> argument_names_field;
  typedef esbmct::field_traits<bool, code_data, &code_data::ellipsis> ellipsis_field;
  typedef esbmct::type2t_traits<arguments_field, ret_type_field, argument_names_field, ellipsis_field> traits;
};

class array_data : public type2t
{
public:
  array_data(type2t::type_ids id, const type2tc &st, const expr2tc &sz, bool i)
    : type2t(id), subtype(st), array_size(sz), size_is_infinite(i) { }
  array_data(const array_data &ref)
    : type2t(ref), subtype(ref.subtype), array_size(ref.array_size),
      size_is_infinite(ref.size_is_infinite) { }

  type2tc subtype;
  expr2tc array_size;
  bool size_is_infinite;

// Type mangling:
  typedef esbmct::field_traits<type2tc, array_data, &array_data::subtype> subtype_field;
  typedef esbmct::field_traits<expr2tc, array_data, &array_data::array_size> array_size_field;
  typedef esbmct::field_traits<bool, array_data, &array_data::size_is_infinite> size_is_infinite_field;
  typedef esbmct::type2t_traits<subtype_field, array_size_field, size_is_infinite_field> traits;
};

class pointer_data : public type2t
{
public:
  pointer_data(type2t::type_ids id, const type2tc &st)
    : type2t(id), subtype(st) { }
  pointer_data(const pointer_data &ref)
    : type2t(ref), subtype(ref.subtype) { }

  type2tc subtype;

// Type mangling:
  typedef esbmct::field_traits<type2tc, pointer_data, &pointer_data::subtype> subtype_field;
  typedef esbmct::type2t_traits<subtype_field> traits;
};

class fixedbv_data : public type2t
{
public:
  fixedbv_data(type2t::type_ids id, unsigned int w, unsigned int ib)
    : type2t(id), width(w), integer_bits(ib) { }
  fixedbv_data(const fixedbv_data &ref)
    : type2t(ref), width(ref.width), integer_bits(ref.integer_bits) { }

  unsigned int width;
  unsigned int integer_bits;

// Type mangling:
  typedef esbmct::field_traits<unsigned int, fixedbv_data, &fixedbv_data::width> width_field;
  typedef esbmct::field_traits<unsigned int, fixedbv_data, &fixedbv_data::integer_bits> integer_bits_field;
  typedef esbmct::type2t_traits<width_field, integer_bits_field> traits;
};

class string_data : public type2t
{
public:
  string_data(type2t::type_ids id, unsigned int w)
    : type2t(id), width(w) { }
  string_data(const string_data &ref)
    : type2t(ref), width(ref.width) { }

  unsigned int width;

// Type mangling:
  typedef esbmct::field_traits<unsigned int, string_data, &string_data::width> width_field;
  typedef esbmct::type2t_traits<width_field> traits;
};

class cpp_name_data : public type2t
{
public:
  cpp_name_data(type2t::type_ids id, const irep_idt &n,
                const std::vector<type2tc> &templ_args)
    : type2t(id), name(n), template_args(templ_args) { }
  cpp_name_data(const cpp_name_data &ref)
    : type2t(ref), name(ref.name), template_args(ref.template_args) { }

  irep_idt name;
  std::vector<type2tc> template_args;

// Type mangling:
  typedef esbmct::field_traits<irep_idt, cpp_name_data, &cpp_name_data::name> name_field;
  typedef esbmct::field_traits<std::vector<type2tc>, cpp_name_data, &cpp_name_data::template_args> template_args_field;
  typedef esbmct::type2t_traits<name_field, template_args_field> traits;
};

// Then give them a typedef name

typedef esbmct::type_methods2<bool_type2t, type2t, typename esbmct::type2t_default_traits::type> bool_type_methods;
typedef esbmct::type_methods2<empty_type2t, type2t, typename esbmct::type2t_default_traits::type> empty_type_methods;
typedef esbmct::type_methods2<symbol_type2t, symbol_type_data, symbol_type_data::traits::type> symbol_type_methods;
typedef esbmct::type_methods2<struct_type2t, struct_union_data, struct_union_data::traits::type> struct_type_methods;
typedef esbmct::type_methods2<union_type2t, struct_union_data, struct_union_data::traits::type> union_type_methods;
typedef esbmct::type_methods2<unsignedbv_type2t, bv_data, bv_data::traits::type> unsignedbv_type_methods;
typedef esbmct::type_methods2<signedbv_type2t, bv_data, bv_data::traits::type> signedbv_type_methods;
typedef esbmct::type_methods2<code_type2t, code_data, code_data::traits::type> code_type_methods;
typedef esbmct::type_methods2<array_type2t, array_data, array_data::traits::type> array_type_methods;
typedef esbmct::type_methods2<pointer_type2t, pointer_data, pointer_data::traits::type> pointer_type_methods;
typedef esbmct::type_methods2<fixedbv_type2t, fixedbv_data, fixedbv_data::traits::type> fixedbv_type_methods;
typedef esbmct::type_methods2<string_type2t, string_data, string_data::traits::type> string_type_methods;
typedef esbmct::type_methods2<cpp_name_type2t, cpp_name_data, cpp_name_data::traits::type> cpp_name_type_methods;

/** Boolean type.
 *  Identifies a boolean type. Contains no additional data.
 *  @extends typet
 */
class bool_type2t : public bool_type_methods
{
public:
  bool_type2t(void) : bool_type_methods (bool_id) {}
  bool_type2t(const bool_type2t &ref) : bool_type_methods(ref) {}
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Empty type.
 *  For void pointers and the like, with no type. No extra data.
 *  @extends type2t
 */
class empty_type2t : public empty_type_methods
{
public:
  empty_type2t(void) : empty_type_methods(empty_id) {}
  empty_type2t(const empty_type2t &ref) : empty_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Symbolic type.
 *  Temporary, prior to linking up types after parsing, or when a struct/array
 *  contains a recursive pointer to its own type.
 *  @extends symbol_type_data
 */
class symbol_type2t : public symbol_type_methods
{
public:
  /** Primary constructor. @param sym_name Name of symbolic type. */
  symbol_type2t(const dstring sym_name) :
    symbol_type_methods(symbol_id, sym_name) { }
  symbol_type2t(const symbol_type2t &ref) :
    symbol_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Struct type.
 *  Represents both C structs and the data in C++ classes. Contains a vector
 *  of types recording what type each member is, a vector of names recording
 *  what the member names are, and a name for the struct.
 *  @extends struct_union_data
 */
class struct_type2t : public struct_type_methods
{
public:
  /** Primary constructor.
   *  @param members Vector of types for the members in this struct.
   *  @param memb_names Vector of names for the members in this struct.
   *  @param name Name of this struct.
   */
  struct_type2t(std::vector<type2tc> &members, std::vector<irep_idt> memb_names,
                irep_idt name)
    : struct_type_methods(struct_id, members, memb_names, name) {}
  struct_type2t(const struct_type2t &ref) : struct_type_methods(ref) {}
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Union type.
 *  Represents a union type - in a similar vein to struct_type2t, this contains
 *  a vector of types and vector of names, each element of which corresponds to
 *  a member in the union. There's also a name for the union.
 *  @extends struct_union_data
 */
class union_type2t : public union_type_methods
{
public:
  /** Primary constructor.
   *  @param members Vector of types corresponding to each member of union.
   *  @param memb_names Vector of names corresponding to each member of union.
   *  @param name Name of this union
   */
  union_type2t(std::vector<type2tc> &members, std::vector<irep_idt> memb_names,
                irep_idt name)
    : union_type_methods(union_id, members, memb_names, name) {}
  union_type2t(const union_type2t &ref) : union_type_methods(ref) {}
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Unsigned integer type.
 *  Represents any form of unsigned integer; the size of this integer is
 *  recorded in the width field.
 *  @extends bv_data
 */
class unsignedbv_type2t : public unsignedbv_type_methods
{
public:
  /** Primary constructor. @param width Width of represented integer */
  unsignedbv_type2t(unsigned int width)
    : unsignedbv_type_methods(unsignedbv_id, width) { }
  unsignedbv_type2t(const unsignedbv_type2t &ref)
    : unsignedbv_type_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

/** Signed integer type.
 *  Represents any form of signed integer; the size of this integer is
 *  recorded in the width field.
 *  @extends bv_data
 */
class signedbv_type2t : public signedbv_type_methods
{
public:
  /** Primary constructor. @param width Width of represented integer */
  signedbv_type2t(signed int width)
    : signedbv_type_methods(signedbv_id, width) { }
  signedbv_type2t(const signedbv_type2t &ref)
    : signedbv_type_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

/** Empty type. For void pointers and the like, with no type. No extra data */
class code_type2t : public code_type_methods
{
public:
  code_type2t(const std::vector<type2tc> &args, const type2tc &ret_type,
              const std::vector<irep_idt> &names, bool e)
    : code_type_methods(code_id, args, ret_type, names, e)
  { assert(args.size() == names.size()); }
  code_type2t(const code_type2t &ref) : code_type_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

/** Array type.
 *  Comes with a subtype of the array and a size that might be constant, might
 *  be nondeterministic, might be infinite. These facts are recorded in the
 *  array_size and size_is_infinite fields.
 *
 *  If size_is_infinite is true, array_size will be null. If array_size is
 *  not a constant number, then it's a dynamically sized array.
 *  @extends array_data
 */
class array_type2t : public array_type_methods
{
public:
  /** Primary constructor.
   *  @param subtype Type of elements in this array.
   *  @param size Size of this array.
   *  @param inf Whether or not this array is infinitely sized
   */
  array_type2t(const type2tc subtype, const expr2tc size, bool inf)
    : array_type_methods (array_id, subtype, size, inf) {
      // If we can simplify the array size, do so
      // XXX, this is probably massively inefficient. Some kind of boundry in
      // the checking process should exist to eliminate this requirement.
      if (!is_nil_expr(size)) {
        expr2tc sz = size->simplify();
        if (!is_nil_expr(sz))
          array_size = sz;
      }
    }
  array_type2t(const array_type2t &ref)
    : array_type_methods(ref) { }

  virtual unsigned int get_width(void) const;

  /** Exception for invalid manipulations of an infinitely sized array. No
   *  actual data stored. */
  class inf_sized_array_excp {
  };

  /** Exception for invalid manipultions of dynamically sized arrays.
   *  Stores the size of the array in the exception; this way the catcher
   *  has it immediately to hand. */
  class dyn_sized_array_excp {
  public:
    dyn_sized_array_excp(const expr2tc _size) : size(_size) {}
    expr2tc size;
  };

  static std::string field_names[esbmct::num_type_fields];
};

/** Pointer type.
 *  Simply has a subtype, of what it points to. No other attributes.
 *  @extends pointer_data
 */
class pointer_type2t : public pointer_type_methods
{
public:
  /** Primary constructor. @param subtype Subtype of this pointer */
  pointer_type2t(const type2tc subtype)
    : pointer_type_methods(pointer_id, subtype) { }
  pointer_type2t(const pointer_type2t &ref)
    : pointer_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Fixed bitvector type.
 *  Contains a spec for a fixed bitwidth number -- this is the equivalent of a
 *  fixedbv_spect in the old irep situation. Stores how bits are distributed
 *  over integer bits and fraction bits.
 *  @extend fixedbv_data
 */
class fixedbv_type2t : public fixedbv_type_methods
{
public:
  /** Primary constructor.
   *  @param width Total number of bits in this type of fixedbv
   *  @param integer Number of integer bits in this type of fixedbv
   */
  fixedbv_type2t(unsigned int width, unsigned int integer)
    : fixedbv_type_methods(fixedbv_id, width, integer) { }
  fixedbv_type2t(const fixedbv_type2t &ref)
    : fixedbv_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** String type class.
 *  Slightly artificial as original irep had no type for this; Represents the
 *  type of a string constant. Because it needs a bit width, we also store the
 *  size of the constant string in elements.
 *  @extends string_data
 */
class string_type2t : public string_type_methods
{
public:
  /** Primary constructor.
   *  @param elements Number of 8-bit characters in string constant.
   */
  string_type2t(unsigned int elements)
    : string_type_methods(string_id, elements) { }
  string_type2t(const string_type2t &ref)
    : string_type_methods(ref) { }
  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** C++ Name type.
 *  Contains a type name, but also a vector of template parameters.
 *  Something in the C++ frontend uses this; it's precise purpose is unclear.
 *  @extends cpp_name_data
 */
class cpp_name_type2t : public cpp_name_type_methods
{
public:
  /** Primary constructor.
   *  @param n Name of this type.
   *  @param ta Vector of template arguments (types).
   */
  cpp_name_type2t(const irep_idt &n, const std::vector<type2tc> &ta)
    : cpp_name_type_methods(cpp_name_id, n, ta){}
  cpp_name_type2t(const cpp_name_type2t &ref)
    : cpp_name_type_methods(ref) { }

  virtual unsigned int get_width(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

// Generate some "is-this-a-blah" macros, and type conversion macros. This is
// fine in terms of using/ keywords in syntax, because the preprocessor
// preprocesses everything out.
#define type_macros(name) \
  inline bool is_##name##_type(const expr2tc &e) \
    { return e->type->type_id == type2t::name##_id; } \
  inline bool is_##name##_type(const type2tc &t) \
    { return t->type_id == type2t::name##_id; } \
  inline const name##_type2t & to_##name##_type(const type2tc &t) \
    { return dynamic_cast<const name##_type2t &> (*t.get()); } \
  inline name##_type2t & to_##name##_type(type2tc &t) \
    { return dynamic_cast<name##_type2t &> (*t.get()); }

type_macros(bool);
type_macros(empty);
type_macros(symbol);
type_macros(struct);
type_macros(union);
type_macros(code);
type_macros(array);
type_macros(pointer);
type_macros(unsignedbv);
type_macros(signedbv);
type_macros(fixedbv);
type_macros(string);
type_macros(cpp_name);
#undef type_macros
#ifdef dynamic_cast
#undef dynamic_cast
#endif

/** Test whether type is an integer. */
inline bool is_bv_type(const type2tc &t) \
{ return (t->type_id == type2t::unsignedbv_id ||
          t->type_id == type2t::signedbv_id); }
inline bool is_bv_type(const expr2tc &e)
{ return is_bv_type(e->type); }

/** Test whether type is a number type - bv or fixedbv. */
inline bool is_number_type(const type2tc &t) \
{ return (t->type_id == type2t::unsignedbv_id ||
          t->type_id == type2t::signedbv_id ||
          t->type_id == type2t::fixedbv_id); }
inline bool is_number_type(const expr2tc &e)
{ return is_number_type(e->type); }

inline bool is_scalar_type(const type2tc &t)
{ return is_number_type(t) || is_pointer_type(t) || is_bool_type(t) ||
         is_empty_type(t) || is_code_type(t); }

inline bool is_scalar_type(const expr2tc &e)
{ return is_scalar_type(e->type); }

inline bool is_multi_dimensional_array(const type2tc &t) {
  if (is_array_type(t)) {
    const array_type2t &arr_type = to_array_type(t);
    return is_array_type(arr_type.subtype);
  } else {
    return false;
  }
}

inline bool is_multi_dimensional_array(const expr2tc &e) {
  return is_multi_dimensional_array(e->type);
}

/** Pool for caching converted types.
 *  Various common types (bool, empty for example) needn't be reallocated
 *  every time we need a new one; it's better to have some global constants
 *  of them, which is what this class provides. There are global bool and empty
 *  types to be used; in addition, there are helper methods to create integer
 *  types with common bit widths, and methods to enter a used type into a cache
 *  of them, allowing migration of typet <=> type2t to be faster.
 */
class type_poolt {
public:
  type_poolt(void);
  type_poolt(bool yolo);

  type2tc bool_type;
  type2tc empty_type;

  const type2tc &get_bool() const { return bool_type; }
  const type2tc &get_empty() const { return empty_type; }

  // For other types, have a pool of them for quick lookup.
  std::map<typet, type2tc> struct_map;
  std::map<typet, type2tc> union_map;
  std::map<typet, type2tc> array_map;
  std::map<typet, type2tc> pointer_map;
  std::map<typet, type2tc> unsignedbv_map;
  std::map<typet, type2tc> signedbv_map;
  std::map<typet, type2tc> fixedbv_map;
  std::map<typet, type2tc> string_map;
  std::map<typet, type2tc> symbol_map;
  std::map<typet, type2tc> code_map;

  // And refs to some of those for /really/ quick lookup;
  const type2tc *uint8;
  const type2tc *uint16;
  const type2tc *uint32;
  const type2tc *uint64;
  const type2tc *int8;
  const type2tc *int16;
  const type2tc *int32;
  const type2tc *int64;

  // Some accessors.
  const type2tc &get_struct(const typet &val);
  const type2tc &get_union(const typet &val);
  const type2tc &get_array(const typet &val);
  const type2tc &get_pointer(const typet &val);
  const type2tc &get_unsignedbv(const typet &val);
  const type2tc &get_signedbv(const typet &val);
  const type2tc &get_fixedbv(const typet &val);
  const type2tc &get_string(const typet &val);
  const type2tc &get_symbol(const typet &val);
  const type2tc &get_code(const typet &val);

  const type2tc &get_uint(unsigned int size);
  const type2tc &get_int(unsigned int size);

  const type2tc &get_uint8() const { return *uint8; }
  const type2tc &get_uint16() const { return *uint16; }
  const type2tc &get_uint32() const { return *uint32; }
  const type2tc &get_uint64() const { return *uint64; }
  const type2tc &get_int8() const { return *int8; }
  const type2tc &get_int16() const { return *int16; }
  const type2tc &get_int32() const { return *int32; }
  const type2tc &get_int64() const { return *int64; }
};

extern type_poolt type_pool;

// Start of definitions for expressions. Forward decs,

class constant2t;
class constant_int2t;
class constant_fixedbv2t;
class constant_bool2t;
class constant_string2t;
class constant_datatype2t;
class constant_struct2t;
class constant_union2t;
class constant_array2t;
class constant_array_of2t;
class symbol2t;
class typecast2t;
class if2t;
class equality2t;
class notequal2t;
class lessthan2t;
class greaterthan2t;
class lessthanequal2t;
class greaterthanequal2t;
class not2t;
class and2t;
class or2t;
class xor2t;
class implies2t;
class bitand2t;
class bitor2t;
class bitxor2t;
class bitnand2t;
class bitnor2t;
class bitnxor2t;
class lshr2t;
class bitnot2t;
class neg2t;
class abs2t;
class add2t;
class sub2t;
class mul2t;
class div2t;
class modulus2t;
class shl2t;
class ashr2t;
class same_object2t;
class pointer_offset2t;
class pointer_object2t;
class address_of2t;
class byte_extract2t;
class byte_update2t;
class with2t;
class member2t;
class index2t;
class zero_string2t;
class zero_length_string2t;
class isnan2t;
class overflow2t;
class overflow_cast2t;
class overflow_neg2t;
class unknown2t;
class invalid2t;
class null_object2t;
class dynamic_object2t;
class dereference2t;
class valid_object2t;
class deallocated_obj2t;
class dynamic_size2t;
class sideeffect2t;
class code_block2t;
class code_assign2t;
class code_init2t;
class code_decl2t;
class code_printf2t;
class code_expression2t;
class code_return2t;
class code_skip2t;
class code_free2t;
class code_goto2t;
class object_descriptor2t;
class code_function_call2t;
class code_comma2t;
class invalid_pointer2t;
class buffer_size2t;
class code_asm2t;
class code_cpp_del_array2t;
class code_cpp_delete2t;
class code_cpp_catch2t;
class code_cpp_throw2t;
class code_cpp_throw_decl2t;
class code_cpp_throw_decl_end2t;
class isinf2t;
class isnormal2t;
class concat2t;

// Data definitions.

class constant2t : public expr2t
{
public:
  constant2t(const type2tc &t, expr2t::expr_ids id) : expr2t(t, id) { }
  constant2t(const constant2t &ref) : expr2t(ref) { }
};

class constant_int_data : public constant2t
{
public:
  constant_int_data(const type2tc &t, expr2t::expr_ids id, const BigInt &bint)
    : constant2t(t, id), constant_value(bint) { }
  constant_int_data(const constant_int_data &ref)
    : constant2t(ref), constant_value(ref.constant_value) { }

  BigInt constant_value;

// Type mangling:
  typedef esbmct::field_traits<BigInt, constant_int_data, &constant_int_data::constant_value> constant_value_field;
  typedef esbmct::expr2t_traits<constant_value_field> traits;
};

class constant_fixedbv_data : public constant2t
{
public:
  constant_fixedbv_data(const type2tc &t, expr2t::expr_ids id,
                        const fixedbvt &fbv)
    : constant2t(t, id), value(fbv) { }
  constant_fixedbv_data(const constant_fixedbv_data &ref)
    : constant2t(ref), value(ref.value) { }

  fixedbvt value;

// Type mangling:
  typedef esbmct::field_traits<fixedbvt, constant_fixedbv_data, &constant_fixedbv_data::value> value_field;
  typedef esbmct::expr2t_traits<value_field> traits;
};

class constant_datatype_data : public constant2t
{
public:
  constant_datatype_data(const type2tc &t, expr2t::expr_ids id,
                         const std::vector<expr2tc> &m)
    : constant2t(t, id), datatype_members(m) { }
  constant_datatype_data(const constant_datatype_data &ref)
    : constant2t(ref), datatype_members(ref.datatype_members) { }

  std::vector<expr2tc> datatype_members;

// Type mangling:
  typedef esbmct::field_traits<std::vector<expr2tc>, constant_datatype_data, &constant_datatype_data::datatype_members> datatype_members_field;
  typedef esbmct::expr2t_traits<datatype_members_field> traits;
};

class constant_bool_data : public constant2t
{
public:
  constant_bool_data(const type2tc &t, expr2t::expr_ids id, bool value)
    : constant2t(t, id), constant_value(value) { }
  constant_bool_data(const constant_bool_data &ref)
    : constant2t(ref), constant_value(ref.constant_value) { }

  bool constant_value;

// Type mangling:
  typedef esbmct::field_traits<bool, constant_bool_data, &constant_bool_data::constant_value> constant_value_field;
  typedef esbmct::expr2t_traits<constant_value_field> traits;
};

class constant_array_of_data : public constant2t
{
public:
  constant_array_of_data(const type2tc &t, expr2t::expr_ids id, expr2tc value)
    : constant2t(t, id), initializer(value) { }
  constant_array_of_data(const constant_array_of_data &ref)
    : constant2t(ref), initializer(ref.initializer) { }

  expr2tc initializer;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, constant_array_of_data, &constant_array_of_data::initializer> initializer_field;
  typedef esbmct::expr2t_traits<initializer_field> traits;
};

class constant_string_data : public constant2t
{
public:
  constant_string_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &v)
    : constant2t(t, id), value(v) { }
  constant_string_data(const constant_string_data &ref)
    : constant2t(ref), value(ref.value) { }

  irep_idt value;

// Type mangling:
  typedef esbmct::field_traits<irep_idt, constant_string_data, &constant_string_data::value> value_field;
  typedef esbmct::expr2t_traits<value_field> traits;
};

class symbol_data : public expr2t
{
public:
  enum renaming_level {
    level0,
    level1,
    level2,
    level1_global,
    level2_global
  };

  symbol_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &v,
              renaming_level lev, unsigned int l1, unsigned int l2,
              unsigned int tr, unsigned int node)
    : expr2t(t, id), thename(v), rlevel(lev), level1_num(l1), level2_num(l2),
      thread_num(tr), node_num(node) { }
  symbol_data(const symbol_data &ref)
    : expr2t(ref), thename(ref.thename), rlevel(ref.rlevel),
      level1_num(ref.level1_num), level2_num(ref.level2_num),
      thread_num(ref.thread_num), node_num(ref.node_num) { }

  virtual std::string get_symbol_name(void) const;

  // So: I want to make this private, however then all the templates accessing
  // it can't access it; and the typedef for symbol_expr_methods further down
  // can't access it too, no matter how many friends I add.
  irep_idt thename;
  renaming_level rlevel;
  unsigned int level1_num; // Function activation record
  unsigned int level2_num; // SSA variable number
  unsigned int thread_num;
  unsigned int node_num;

// Type mangling:
  typedef esbmct::field_traits<irep_idt, symbol_data, &symbol_data::thename> thename_field;
  typedef esbmct::field_traits<renaming_level, symbol_data, &symbol_data::rlevel> rlevel_field;
  typedef esbmct::field_traits<unsigned int, symbol_data, &symbol_data::level1_num> level1_num_field;
  typedef esbmct::field_traits<unsigned int, symbol_data, &symbol_data::level2_num> level2_num_field;
  typedef esbmct::field_traits<unsigned int, symbol_data, &symbol_data::thread_num> thread_num_field;
  typedef esbmct::field_traits<unsigned int, symbol_data, &symbol_data::node_num> node_num_field;
  typedef esbmct::expr2t_traits<thename_field, rlevel_field, level1_num_field, level2_num_field, thread_num_field, node_num_field> traits;
};

class typecast_data : public expr2t
{
public:
  typecast_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : expr2t(t, id), from(v) { }
  typecast_data(const typecast_data &ref)
    : expr2t(ref), from(ref.from) { }

  expr2tc from;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, typecast_data, &typecast_data::from> from_field;
  typedef esbmct::expr2t_traits<from_field> traits;
};

class if_data : public expr2t
{
public:
  if_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &c,
                const expr2tc &tv, const expr2tc &fv)
    : expr2t(t, id), cond(c), true_value(tv), false_value(fv) { }
  if_data(const if_data &ref)
    : expr2t(ref), cond(ref.cond), true_value(ref.true_value),
      false_value(ref.false_value) { }

  expr2tc cond;
  expr2tc true_value;
  expr2tc false_value;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, if_data, &if_data::cond> cond_field;
  typedef esbmct::field_traits<expr2tc, if_data, &if_data::true_value> true_value_field;
  typedef esbmct::field_traits<expr2tc, if_data, &if_data::false_value> false_value_field;
  typedef esbmct::expr2t_traits<cond_field, true_value_field, false_value_field> traits;
};

class relation_data : public expr2t
{
  public:
  relation_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
                const expr2tc &s2)
    : expr2t(t, id), side_1(s1), side_2(s2) { }
  relation_data(const relation_data &ref)
    : expr2t(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, relation_data, &relation_data::side_1> side_1_field;
  typedef esbmct::field_traits<expr2tc, relation_data, &relation_data::side_2> side_2_field;
  typedef esbmct::expr2t_traits<side_1_field, side_2_field> traits;
};

class logical_ops : public expr2t
{
public:
  logical_ops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  logical_ops(const logical_ops &ref)
    : expr2t(ref) { }
};

class not_data : public logical_ops
{
public:
  not_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : logical_ops(t, id), value(v) { }
  not_data(const not_data &ref)
    : logical_ops(ref), value(ref.value) { }

  expr2tc value;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, not_data, &not_data::value> value_field;
  typedef esbmct::expr2t_traits_always_construct<value_field> traits;
};

class logic_2ops : public logical_ops
{
public:
  logic_2ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
             const expr2tc &s2)
    : logical_ops(t, id), side_1(s1), side_2(s2) { }
  logic_2ops(const logic_2ops &ref)
    : logical_ops(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, logic_2ops, &logic_2ops::side_1> side_1_field;
  typedef esbmct::field_traits<expr2tc, logic_2ops, &logic_2ops::side_2> side_2_field;
  typedef esbmct::expr2t_traits<side_1_field, side_2_field> traits;
};

class bitops : public expr2t
{
public:
  bitops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  bitops(const bitops &ref)
    : expr2t(ref) { }
};

class bitnot_data : public bitops
{
public:
  bitnot_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : bitops(t, id), value(v) { }
  bitnot_data(const bitnot_data &ref)
    : bitops(ref), value(ref.value) { }

  expr2tc value;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, bitnot_data, &bitnot_data::value> value_field;
  typedef esbmct::expr2t_traits<value_field> traits;
};

class bit_2ops : public bitops
{
public:
  bit_2ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
           const expr2tc &s2)
    : bitops(t, id), side_1(s1), side_2(s2) { }
  bit_2ops(const bit_2ops &ref)
    : bitops(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, bit_2ops, &bit_2ops::side_1> side_1_field;
  typedef esbmct::field_traits<expr2tc, bit_2ops, &bit_2ops::side_2> side_2_field;
  typedef esbmct::expr2t_traits<side_1_field, side_2_field> traits;
};

class arith_ops : public expr2t
{
public:
  arith_ops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  arith_ops(const arith_ops &ref)
    : expr2t(ref) { }
};

class arith_1op : public arith_ops
{
public:
  arith_1op(const type2tc &t, arith_ops::expr_ids id, const expr2tc &v)
    : arith_ops(t, id), value(v) { }
  arith_1op(const arith_1op &ref)
    : arith_ops(ref), value(ref.value) { }

  expr2tc value;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, arith_1op, &arith_1op::value> value_field;
  typedef esbmct::expr2t_traits<value_field> traits;
};

class arith_2ops : public arith_ops
{
public:
  arith_2ops(const type2tc &t, arith_ops::expr_ids id, const expr2tc &v1,
             const expr2tc &v2)
    : arith_ops(t, id), side_1(v1), side_2(v2) { }
  arith_2ops(const arith_2ops &ref)
    : arith_ops(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, arith_2ops, &arith_2ops::side_1> side_1_field;
  typedef esbmct::field_traits<expr2tc, arith_2ops, &arith_2ops::side_2> side_2_field;
  typedef esbmct::expr2t_traits<side_1_field, side_2_field> traits;
};

class same_object_data : public expr2t
{
public:
  same_object_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v1,
                   const expr2tc &v2)
    : expr2t(t, id), side_1(v1), side_2(v2) { }
  same_object_data(const same_object_data &ref)
    : expr2t(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, same_object_data, &same_object_data::side_1> side_1_field;
  typedef esbmct::field_traits<expr2tc, same_object_data, &same_object_data::side_2> side_2_field;
  typedef esbmct::expr2t_traits<side_1_field, side_2_field> traits;
};

class pointer_ops : public expr2t
{
public:
  pointer_ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &p)
    : expr2t(t, id), ptr_obj(p) { }
  pointer_ops(const pointer_ops &ref)
    : expr2t(ref), ptr_obj(ref.ptr_obj) { }

  expr2tc ptr_obj;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, pointer_ops, &pointer_ops::ptr_obj> ptr_obj_field;
  typedef esbmct::expr2t_traits<ptr_obj_field> traits;
};

// Special class for invalid_pointer2t, which needs always-construct forcing
class invalid_pointer_ops : public pointer_ops
{
public:
  // Forward constructors downwards
  invalid_pointer_ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &p)
    : pointer_ops(t, id, p) { }
  invalid_pointer_ops(const invalid_pointer_ops &ref)
    : pointer_ops(ref) { }

// Type mangling:
  typedef esbmct::expr2t_traits_always_construct<ptr_obj_field> traits;
};

class byte_ops : public expr2t
{
public:
  byte_ops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id){ }
  byte_ops(const byte_ops &ref)
    : expr2t(ref) { }
};

class byte_extract_data : public byte_ops
{
public:
  byte_extract_data(const type2tc &t, expr2t::expr_ids id,
                    const expr2tc &s, const expr2tc &o, bool be)
    : byte_ops(t, id), source_value(s), source_offset(o), big_endian(be) { }
  byte_extract_data(const byte_extract_data &ref)
    : byte_ops(ref), source_value(ref.source_value),
      source_offset(ref.source_offset), big_endian(ref.big_endian) { }

  expr2tc source_value;
  expr2tc source_offset;
  bool big_endian;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, byte_extract_data, &byte_extract_data::source_value> source_value_field;
  typedef esbmct::field_traits<expr2tc, byte_extract_data, &byte_extract_data::source_offset> source_offset_field;
  typedef esbmct::field_traits<bool, byte_extract_data, &byte_extract_data::big_endian> big_endian_field;
  typedef esbmct::expr2t_traits<source_value_field, source_offset_field, big_endian_field> traits;
};

class byte_update_data : public byte_ops
{
public:
  byte_update_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &s,
                   const expr2tc &o, const expr2tc &v, bool be)
    : byte_ops(t, id), source_value(s), source_offset(o), update_value(v),
      big_endian(be) { }
  byte_update_data(const byte_update_data &ref)
    : byte_ops(ref), source_value(ref.source_value),
      source_offset(ref.source_offset), update_value(ref.update_value),
      big_endian(ref.big_endian) { }

  expr2tc source_value;
  expr2tc source_offset;
  expr2tc update_value;
  bool big_endian;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, byte_update_data, &byte_update_data::source_value> source_value_field;
  typedef esbmct::field_traits<expr2tc, byte_update_data, &byte_update_data::source_offset> source_offset_field;
  typedef esbmct::field_traits<expr2tc, byte_update_data, &byte_update_data::update_value> update_value_field;
  typedef esbmct::field_traits<bool, byte_update_data, &byte_update_data::big_endian> big_endian_field;
  typedef esbmct::expr2t_traits<source_value_field, source_offset_field, update_value_field, big_endian_field> traits;
};

class datatype_ops : public expr2t
{
public:
  datatype_ops(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  datatype_ops(const datatype_ops &ref)
    : expr2t(ref) { }
};

class with_data : public datatype_ops
{
public:
  with_data(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &sv,
            const expr2tc &uf, const expr2tc &uv)
    : datatype_ops(t, id), source_value(sv), update_field(uf), update_value(uv)
      { }
  with_data(const with_data &ref)
    : datatype_ops(ref), source_value(ref.source_value),
      update_field(ref.update_field), update_value(ref.update_value)
      { }

  expr2tc source_value;
  expr2tc update_field;
  expr2tc update_value;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, with_data, &with_data::source_value> source_value_field;
  typedef esbmct::field_traits<expr2tc, with_data, &with_data::update_field> update_field_field;
  typedef esbmct::field_traits<expr2tc, with_data, &with_data::update_value> update_value_field;
  typedef esbmct::expr2t_traits<source_value_field, update_field_field, update_value_field> traits;
};

class member_data : public datatype_ops
{
public:
  member_data(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &sv,
              const irep_idt &m)
    : datatype_ops(t, id), source_value(sv), member(m) { }
  member_data(const member_data &ref)
    : datatype_ops(ref), source_value(ref.source_value), member(ref.member) { }

  expr2tc source_value;
  irep_idt member;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, member_data, &member_data::source_value> source_value_field;
  typedef esbmct::field_traits<irep_idt, member_data, &member_data::member> member_field;
  typedef esbmct::expr2t_traits<source_value_field, member_field> traits;
};

class index_data : public datatype_ops
{
public:
  index_data(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &sv,
              const expr2tc &i)
    : datatype_ops(t, id), source_value(sv), index(i) { }
  index_data(const index_data &ref)
    : datatype_ops(ref), source_value(ref.source_value), index(ref.index) { }

  expr2tc source_value;
  expr2tc index;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, index_data, &index_data::source_value> source_value_field;
  typedef esbmct::field_traits<expr2tc, index_data, &index_data::index> index_field;
  typedef esbmct::expr2t_traits<source_value_field, index_field> traits;
};

class string_ops : public expr2t
{
public:
  string_ops(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &s)
    : expr2t(t, id), string(s) { }
  string_ops(const string_ops &ref)
    : expr2t(ref), string(ref.string) { }

  expr2tc string;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, string_ops, &string_ops::string> string_field;
  typedef esbmct::expr2t_traits<string_field> traits;
};

class isnan_data : public expr2t
{
public:
  isnan_data(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &in)
    : expr2t(t, id), value(in) { }
  isnan_data(const isnan_data &ref)
    : expr2t(ref), value(ref.value) { }

  expr2tc value;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, isnan_data, &isnan_data::value> value_field;
  typedef esbmct::expr2t_traits<value_field> traits;
};

class overflow_ops : public expr2t
{
public:
  overflow_ops(const type2tc &t, datatype_ops::expr_ids id, const expr2tc &v)
    : expr2t(t, id), operand(v) { }
  overflow_ops(const overflow_ops &ref)
    : expr2t(ref), operand(ref.operand) { }

  expr2tc operand;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, overflow_ops, &overflow_ops::operand> operand_field;
  typedef esbmct::expr2t_traits<operand_field> traits;
};

class overflow_cast_data : public overflow_ops
{
public:
  overflow_cast_data(const type2tc &t, datatype_ops::expr_ids id,
                     const expr2tc &v, unsigned int b)
    : overflow_ops(t, id, v), bits(b) { }
  overflow_cast_data(const overflow_cast_data &ref)
    : overflow_ops(ref), bits(ref.bits) { }

  unsigned int bits;

// Type mangling:
  typedef esbmct::field_traits<unsigned int, overflow_cast_data, &overflow_cast_data::bits> bits_field;
  typedef esbmct::expr2t_traits<bits_field> traits;
};

class dynamic_object_data : public expr2t
{
public:
  dynamic_object_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &i,
                      bool inv, bool unk)
    : expr2t(t, id), instance(i), invalid(inv), unknown(unk) { }
  dynamic_object_data(const dynamic_object_data &ref)
    : expr2t(ref), instance(ref.instance), invalid(ref.invalid),
      unknown(ref.unknown) { }

  expr2tc instance;
  bool invalid;
  bool unknown;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, dynamic_object_data, &dynamic_object_data::instance> instance_field;
  typedef esbmct::field_traits<bool, dynamic_object_data, &dynamic_object_data::invalid> invalid_field;
  typedef esbmct::field_traits<bool, dynamic_object_data, &dynamic_object_data::unknown> unknown_field;
  typedef esbmct::expr2t_traits<instance_field, invalid_field, unknown_field> traits;
};

class dereference_data : public expr2t
{
public:
  dereference_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : expr2t(t, id), value(v) { }
  dereference_data(const dereference_data &ref)
    : expr2t(ref), value(ref.value) { }

  expr2tc value;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, dereference_data, &dereference_data::value> value_field;
  typedef esbmct::expr2t_traits<value_field> traits;
};

class object_ops : public expr2t
{
public:
  object_ops(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : expr2t(t, id), value(v) { }
  object_ops(const object_ops &ref)
    : expr2t(ref), value(ref.value) { }

  expr2tc value;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, object_ops, &object_ops::value> value_field;
  typedef esbmct::expr2t_traits_always_construct<value_field> traits;
};

class sideeffect_data : public expr2t
{
public:
  /** Enumeration identifying each particular kind of side effect. The values
   *  themselves are entirely self explanatory. */
  enum allockind {
    malloc,
    alloca,
    cpp_new,
    cpp_new_arr,
    nondet,
    function_call
  };

  sideeffect_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &op,
                  const expr2tc &sz, const std::vector<expr2tc> &args,
                  const type2tc &tp, allockind k)
    : expr2t(t, id), operand(op), size(sz), arguments(args), alloctype(tp),
                     kind(k) { }
  sideeffect_data(const sideeffect_data &ref)
    : expr2t(ref), operand(ref.operand), size(ref.size),
      arguments(ref.arguments), alloctype(ref.alloctype), kind(ref.kind) { }

  expr2tc operand;
  expr2tc size;
  std::vector<expr2tc> arguments;
  type2tc alloctype;
  allockind kind;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, sideeffect_data, &sideeffect_data::operand> operand_field;
  typedef esbmct::field_traits<expr2tc, sideeffect_data, &sideeffect_data::size> size_field;
  typedef esbmct::field_traits<std::vector<expr2tc>, sideeffect_data, &sideeffect_data::arguments> arguments_field;
  typedef esbmct::field_traits<type2tc, sideeffect_data, &sideeffect_data::alloctype> alloctype_field;
  typedef esbmct::field_traits<allockind, sideeffect_data, &sideeffect_data::kind> kind_field;
  typedef esbmct::expr2t_traits<operand_field, size_field, arguments_field, alloctype_field, kind_field> traits;
};

class code_base : public expr2t
{
public:
  code_base(const type2tc &t, expr2t::expr_ids id)
    : expr2t(t, id) { }
  code_base(const code_base &ref)
    : expr2t(ref) { }
};

class code_block_data : public code_base
{
public:
  code_block_data(const type2tc &t, expr2t::expr_ids id,
                  const std::vector<expr2tc> &v)
    : code_base(t, id), operands(v) { }
  code_block_data(const code_block_data &ref)
    : code_base(ref), operands(ref.operands) { }

  std::vector<expr2tc> operands;

// Type mangling:
  typedef esbmct::field_traits<std::vector<expr2tc>, code_block_data, &code_block_data::operands> operands_field;
  typedef esbmct::expr2t_traits<operands_field> traits;
};

class code_assign_data : public code_base
{
public:
  code_assign_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &ta,
                   const expr2tc &s)
    : code_base(t, id), target(ta), source(s) { }
  code_assign_data(const code_assign_data &ref)
    : code_base(ref), target(ref.target), source(ref.source) { }

  expr2tc target;
  expr2tc source;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, code_assign_data, &code_assign_data::target> target_field;
  typedef esbmct::field_traits<expr2tc, code_assign_data, &code_assign_data::source> source_field;
  typedef esbmct::expr2t_traits<target_field, source_field> traits;
};

class code_decl_data : public code_base
{
public:
  code_decl_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &v)
    : code_base(t, id), value(v) { }
  code_decl_data(const code_decl_data &ref)
    : code_base(ref), value(ref.value) { }

  irep_idt value;

// Type mangling:
  typedef esbmct::field_traits<irep_idt, code_decl_data, &code_decl_data::value> value_field;
  typedef esbmct::expr2t_traits<value_field> traits;
};

class code_printf_data : public code_base
{
public:
  code_printf_data(const type2tc &t, expr2t::expr_ids id,
                   const std::vector<expr2tc> &v)
    : code_base(t, id), operands(v) { }
  code_printf_data(const code_printf_data &ref)
    : code_base(ref), operands(ref.operands) { }

  std::vector<expr2tc> operands;

// Type mangling:
  typedef esbmct::field_traits<std::vector<expr2tc>, code_printf_data, &code_printf_data::operands> operands_field;
  typedef esbmct::expr2t_traits<operands_field> traits;
};

class code_expression_data : public code_base
{
public:
  code_expression_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &o)
    : code_base(t, id), operand(o) { }
  code_expression_data(const code_expression_data &ref)
    : code_base(ref), operand(ref.operand) { }

  expr2tc operand;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, code_expression_data, &code_expression_data::operand> operand_field;
  typedef esbmct::expr2t_traits_always_construct<operand_field> traits;
};

class code_goto_data : public code_base
{
public:
  code_goto_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &tg)
    : code_base(t, id), target(tg) { }
  code_goto_data(const code_goto_data &ref)
    : code_base(ref), target(ref.target) { }

  irep_idt target;

// Type mangling:
  typedef esbmct::field_traits<irep_idt, code_goto_data, &code_goto_data::target> target_field;
  typedef esbmct::expr2t_traits<target_field> traits;
};

class object_desc_data : public expr2t
{
  public:
    object_desc_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &o,
                     const expr2tc &offs, unsigned int align)
      : expr2t(t, id), object(o), offset(offs), alignment(align) { }
    object_desc_data(const object_desc_data &ref)
      : expr2t(ref), object(ref.object), offset(ref.offset),
        alignment(ref.alignment) { }

    expr2tc object;
    expr2tc offset;
    unsigned int alignment;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, object_desc_data, &object_desc_data::object> object_field;
  typedef esbmct::field_traits<expr2tc, object_desc_data, &object_desc_data::offset> offset_field;
  typedef esbmct::field_traits<unsigned int, object_desc_data, &object_desc_data::alignment> alignment_field;
  typedef esbmct::expr2t_traits<object_field, offset_field, alignment_field> traits;
};

class code_funccall_data : public code_base
{
public:
  code_funccall_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &r,
                     const expr2tc &func, const std::vector<expr2tc> &ops)
    : code_base(t, id), ret(r), function(func), operands(ops) { }
  code_funccall_data(const code_funccall_data &ref)
    : code_base(ref), ret(ref.ret), function(ref.function),
      operands(ref.operands) { }

  expr2tc ret;
  expr2tc function;
  std::vector<expr2tc> operands;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, code_funccall_data, &code_funccall_data::ret> ret_field;
  typedef esbmct::field_traits<expr2tc, code_funccall_data, &code_funccall_data::function> function_field;
  typedef esbmct::field_traits<std::vector<expr2tc>, code_funccall_data, &code_funccall_data::operands> operands_field;
  typedef esbmct::expr2t_traits<ret_field, function_field, operands_field> traits;
};

class code_comma_data : public code_base
{
public:
  code_comma_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &s1,
                  const expr2tc &s2)
    : code_base(t, id), side_1(s1), side_2(s2) { }
  code_comma_data(const code_comma_data &ref)
    : code_base(ref), side_1(ref.side_1), side_2(ref.side_2) { }

  expr2tc side_1;
  expr2tc side_2;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, code_comma_data, &code_comma_data::side_1> side_1_field;
  typedef esbmct::field_traits<expr2tc, code_comma_data, &code_comma_data::side_2> side_2_field;
  typedef esbmct::expr2t_traits<side_1_field, side_2_field> traits;
};

class buffer_size_data : public expr2t
{
public:
  buffer_size_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &v)
    : expr2t(t, id), value(v) { }
  buffer_size_data(const buffer_size_data &ref)
    : expr2t(ref), value(ref.value) { }

  expr2tc value;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, buffer_size_data, &buffer_size_data::value> value_field;
  typedef esbmct::expr2t_traits<value_field> traits;
};

class code_asm_data : public code_base
{
public:
  code_asm_data(const type2tc &t, expr2t::expr_ids id, const irep_idt &v)
    : code_base(t, id), value(v) { }
  code_asm_data(const code_asm_data &ref)
    : code_base(ref), value(ref.value) { }

  irep_idt value;

// Type mangling:
  typedef esbmct::field_traits<irep_idt, code_asm_data, &code_asm_data::value> value_field;
  typedef esbmct::expr2t_traits<value_field> traits;
};

class code_cpp_catch_data : public code_base
{
public:
  code_cpp_catch_data(const type2tc &t, expr2t::expr_ids id,
                      const std::vector<irep_idt> &el)
    : code_base(t, id), exception_list(el) { }
  code_cpp_catch_data(const code_cpp_catch_data &ref)
    : code_base(ref), exception_list(ref.exception_list) { }

  std::vector<irep_idt> exception_list;

// Type mangling:
  typedef esbmct::field_traits<std::vector<irep_idt>, code_cpp_catch_data, &code_cpp_catch_data::exception_list> exception_list_field;
  typedef esbmct::expr2t_traits<exception_list_field> traits;
};

class code_cpp_throw_data : public code_base
{
public:
  code_cpp_throw_data(const type2tc &t, expr2t::expr_ids id, const expr2tc &o,
                      const std::vector<irep_idt> &l)
    : code_base(t, id), operand(o), exception_list(l) { }
  code_cpp_throw_data(const code_cpp_throw_data &ref)
    : code_base(ref), operand(ref.operand), exception_list(ref.exception_list)
      { }

  expr2tc operand;
  std::vector<irep_idt> exception_list;

// Type mangling:
  typedef esbmct::field_traits<expr2tc, code_cpp_throw_data, &code_cpp_throw_data::operand> operand_field;
  typedef esbmct::field_traits<std::vector<irep_idt>, code_cpp_throw_data, &code_cpp_throw_data::exception_list> exception_list_field;
  typedef esbmct::expr2t_traits<operand_field, exception_list_field> traits;
};

class code_cpp_throw_decl_data : public code_base
{
public:
  code_cpp_throw_decl_data(const type2tc &t, expr2t::expr_ids id,
                           const std::vector<irep_idt> &l)
    : code_base(t, id), exception_list(l) { }
  code_cpp_throw_decl_data(const code_cpp_throw_decl_data &ref)
    : code_base(ref), exception_list(ref.exception_list)
      { }

  std::vector<irep_idt> exception_list;

// Type mangling:
  typedef esbmct::field_traits<std::vector<irep_idt>, code_cpp_throw_decl_data, &code_cpp_throw_decl_data::exception_list> exception_list_field;
  typedef esbmct::expr2t_traits<exception_list_field> traits;
};

class concat_data : public expr2t
{
public:
  concat_data(const type2tc &t, expr2t::expr_ids id,
              const std::vector<expr2tc> &d)
    : expr2t(t, id), data_items(d) { }
  concat_data(const concat_data &ref)
    : expr2t(ref), data_items(ref.data_items)
      { }

  std::vector<expr2tc> data_items;

// Type mangling:
  typedef esbmct::field_traits<std::vector<expr2tc>, concat_data, &concat_data::data_items> data_items_field;
  typedef esbmct::expr2t_traits<data_items_field> traits;
};

// Give everything a typedef name. Use this to construct both the templated
// expression methods, but also the container class which needs the template
// parameters too.
// Given how otherwise this means typing a large amount of template arguments
// again and again, this gets macro'd.

#define irep_typedefs(basename, superclass, type_tag, ...) \
  typedef esbmct::something2tc<basename##2t, expr2t::basename##_id, superclass,\
                               type_tag> basename##2tc; \
  typedef esbmct::expr_methods2<basename##2t, superclass, superclass::traits::type> basename##_expr_methods;

// Special case for some empty ireps,

#define irep_typedefs_empty(basename, superclass, type_tag) \
  typedef esbmct::something2tc<basename##2t, expr2t::basename##_id, superclass,\
                               type_tag> basename##2tc; \
  typedef esbmct::expr_methods2<basename##2t, superclass, esbmct::expr2t_default_traits::type> basename##_expr_methods;

irep_typedefs(constant_int, constant_int_data, esbmct::takestype,
              BigInt, constant_int_data, &constant_int_data::constant_value);
irep_typedefs(constant_fixedbv, constant_fixedbv_data, esbmct::takestype,
              fixedbvt, constant_fixedbv_data, &constant_fixedbv_data::value);
irep_typedefs(constant_struct, constant_datatype_data, esbmct::takestype,
              std::vector<expr2tc>, constant_datatype_data,
              &constant_datatype_data::datatype_members);
irep_typedefs(constant_union, constant_datatype_data, esbmct::takestype,
              std::vector<expr2tc>, constant_datatype_data,
              &constant_datatype_data::datatype_members);
irep_typedefs(constant_array, constant_datatype_data, esbmct::takestype,
              std::vector<expr2tc>, constant_datatype_data,
              &constant_datatype_data::datatype_members);
irep_typedefs(constant_bool, constant_bool_data, esbmct::notype,
              bool, constant_bool_data, &constant_bool_data::constant_value);
irep_typedefs(constant_array_of, constant_array_of_data, esbmct::takestype,
              expr2tc, constant_array_of_data,
              &constant_array_of_data::initializer);
irep_typedefs(constant_string, constant_string_data, esbmct::takestype,
              irep_idt, constant_string_data, &constant_string_data::value);
irep_typedefs(symbol, symbol_data, esbmct::takestype,
              irep_idt, symbol_data, &symbol_data::thename,
              symbol_data::renaming_level, symbol_data, &symbol_data::rlevel,
              unsigned int, symbol_data, &symbol_data::level1_num,
              unsigned int, symbol_data, &symbol_data::level2_num,
              unsigned int, symbol_data, &symbol_data::thread_num,
              unsigned int, symbol_data, &symbol_data::node_num);
irep_typedefs(typecast,typecast_data, esbmct::takestype,
              expr2tc, typecast_data, &typecast_data::from);
irep_typedefs(if, if_data, esbmct::takestype,
              expr2tc, if_data, &if_data::cond,
              expr2tc, if_data, &if_data::true_value,
              expr2tc, if_data, &if_data::false_value);
irep_typedefs(equality, relation_data, esbmct::notype,
              expr2tc, relation_data, &relation_data::side_1,
              expr2tc, relation_data, &relation_data::side_2);
irep_typedefs(notequal, relation_data, esbmct::notype,
              expr2tc, relation_data, &relation_data::side_1,
              expr2tc, relation_data, &relation_data::side_2);
irep_typedefs(lessthan, relation_data, esbmct::notype,
              expr2tc, relation_data, &relation_data::side_1,
              expr2tc, relation_data, &relation_data::side_2);
irep_typedefs(greaterthan, relation_data, esbmct::notype,
              expr2tc, relation_data, &relation_data::side_1,
              expr2tc, relation_data, &relation_data::side_2);
irep_typedefs(lessthanequal, relation_data, esbmct::notype,
              expr2tc, relation_data, &relation_data::side_1,
              expr2tc, relation_data, &relation_data::side_2);
irep_typedefs(greaterthanequal, relation_data, esbmct::notype,
              expr2tc, relation_data, &relation_data::side_1,
              expr2tc, relation_data, &relation_data::side_2);
irep_typedefs(not, not_data, esbmct::notype,
              expr2tc, not_data, &not_data::value);
irep_typedefs(and, logic_2ops, esbmct::notype,
              expr2tc, logic_2ops, &logic_2ops::side_1,
              expr2tc, logic_2ops, &logic_2ops::side_2);
irep_typedefs(or, logic_2ops, esbmct::notype,
              expr2tc, logic_2ops, &logic_2ops::side_1,
              expr2tc, logic_2ops, &logic_2ops::side_2);
irep_typedefs(xor, logic_2ops, esbmct::notype,
              expr2tc, logic_2ops, &logic_2ops::side_1,
              expr2tc, logic_2ops, &logic_2ops::side_2);
irep_typedefs(implies, logic_2ops, esbmct::notype,
              expr2tc, logic_2ops, &logic_2ops::side_1,
              expr2tc, logic_2ops, &logic_2ops::side_2);
irep_typedefs(bitand, bit_2ops, esbmct::takestype,
              expr2tc, bit_2ops, &bit_2ops::side_1,
              expr2tc, bit_2ops, &bit_2ops::side_2);
irep_typedefs(bitor, bit_2ops, esbmct::takestype,
              expr2tc, bit_2ops, &bit_2ops::side_1,
              expr2tc, bit_2ops, &bit_2ops::side_2);
irep_typedefs(bitxor, bit_2ops, esbmct::takestype,
              expr2tc, bit_2ops, &bit_2ops::side_1,
              expr2tc, bit_2ops, &bit_2ops::side_2);
irep_typedefs(bitnand, bit_2ops, esbmct::takestype,
              expr2tc, bit_2ops, &bit_2ops::side_1,
              expr2tc, bit_2ops, &bit_2ops::side_2);
irep_typedefs(bitnor, bit_2ops, esbmct::takestype,
              expr2tc, bit_2ops, &bit_2ops::side_1,
              expr2tc, bit_2ops, &bit_2ops::side_2);
irep_typedefs(bitnxor, bit_2ops, esbmct::takestype,
              expr2tc, bit_2ops, &bit_2ops::side_1,
              expr2tc, bit_2ops, &bit_2ops::side_2);
irep_typedefs(lshr, bit_2ops, esbmct::takestype,
              expr2tc, bit_2ops, &bit_2ops::side_1,
              expr2tc, bit_2ops, &bit_2ops::side_2);
irep_typedefs(bitnot, bitnot_data, esbmct::takestype,
              expr2tc, bitnot_data, &bitnot_data::value);
irep_typedefs(neg, arith_1op, esbmct::takestype,
              expr2tc, arith_1op, &arith_1op::value);
irep_typedefs(abs, arith_1op, esbmct::takestype,
              expr2tc, arith_1op, &arith_1op::value);
irep_typedefs(add, arith_2ops, esbmct::takestype,
              expr2tc, arith_2ops, &arith_2ops::side_1,
              expr2tc, arith_2ops, &arith_2ops::side_2);
irep_typedefs(sub, arith_2ops, esbmct::takestype,
              expr2tc, arith_2ops, &arith_2ops::side_1,
              expr2tc, arith_2ops, &arith_2ops::side_2);
irep_typedefs(mul, arith_2ops, esbmct::takestype,
              expr2tc, arith_2ops, &arith_2ops::side_1,
              expr2tc, arith_2ops, &arith_2ops::side_2);
irep_typedefs(div, arith_2ops, esbmct::takestype,
              expr2tc, arith_2ops, &arith_2ops::side_1,
              expr2tc, arith_2ops, &arith_2ops::side_2);
irep_typedefs(modulus, arith_2ops, esbmct::takestype,
              expr2tc, arith_2ops, &arith_2ops::side_1,
              expr2tc, arith_2ops, &arith_2ops::side_2);
irep_typedefs(shl, arith_2ops, esbmct::takestype,
              expr2tc, arith_2ops, &arith_2ops::side_1,
              expr2tc, arith_2ops, &arith_2ops::side_2);
irep_typedefs(ashr, arith_2ops, esbmct::takestype,
              expr2tc, arith_2ops, &arith_2ops::side_1,
              expr2tc, arith_2ops, &arith_2ops::side_2);
irep_typedefs(same_object, same_object_data, esbmct::notype,
              expr2tc, same_object_data, &same_object_data::side_1,
              expr2tc, same_object_data, &same_object_data::side_2);
irep_typedefs(pointer_offset, pointer_ops, esbmct::takestype,
              expr2tc, pointer_ops, &pointer_ops::ptr_obj);
irep_typedefs(pointer_object, pointer_ops, esbmct::takestype,
              expr2tc, pointer_ops, &pointer_ops::ptr_obj);
irep_typedefs(address_of, pointer_ops, esbmct::takestype,
              expr2tc, pointer_ops, &pointer_ops::ptr_obj);
irep_typedefs(byte_extract, byte_extract_data, esbmct::takestype,
              expr2tc, byte_extract_data, &byte_extract_data::source_value,
              expr2tc, byte_extract_data, &byte_extract_data::source_offset,
              bool, byte_extract_data, &byte_extract_data::big_endian);
irep_typedefs(byte_update, byte_update_data, esbmct::takestype,
              expr2tc, byte_update_data, &byte_update_data::source_value,
              expr2tc, byte_update_data, &byte_update_data::source_offset,
              expr2tc, byte_update_data, &byte_update_data::update_value,
              bool, byte_update_data, &byte_update_data::big_endian);
irep_typedefs(with, with_data, esbmct::takestype,
              expr2tc, with_data, &with_data::source_value,
              expr2tc, with_data, &with_data::update_field,
              expr2tc, with_data, &with_data::update_value);
irep_typedefs(member, member_data, esbmct::takestype,
              expr2tc, member_data, &member_data::source_value,
              irep_idt, member_data, &member_data::member);
irep_typedefs(index, index_data, esbmct::takestype,
              expr2tc, index_data, &index_data::source_value,
              expr2tc, index_data, &index_data::index);
irep_typedefs(zero_string, string_ops, esbmct::notype,
              expr2tc, string_ops, &string_ops::string);
irep_typedefs(zero_length_string, string_ops, esbmct::notype,
              expr2tc, string_ops, &string_ops::string);
irep_typedefs(isnan, isnan_data, esbmct::takestype,
              expr2tc, isnan_data, &isnan_data::value);
irep_typedefs(overflow, overflow_ops, esbmct::takestype,
              expr2tc, overflow_ops, &overflow_ops::operand);
irep_typedefs(overflow_cast, overflow_cast_data, esbmct::takestype,
              expr2tc, overflow_ops, &overflow_ops::operand,
              unsigned int, overflow_cast_data, &overflow_cast_data::bits);
irep_typedefs(overflow_neg, overflow_ops, esbmct::takestype,
              expr2tc, overflow_ops, &overflow_ops::operand);
irep_typedefs_empty(unknown, expr2t, esbmct::takestype);
irep_typedefs_empty(invalid, expr2t, esbmct::takestype);
irep_typedefs_empty(null_object, expr2t, esbmct::takestype);
irep_typedefs(dynamic_object, dynamic_object_data, esbmct::takestype,
              expr2tc, dynamic_object_data, &dynamic_object_data::instance,
              bool, dynamic_object_data, &dynamic_object_data::invalid,
              bool, dynamic_object_data, &dynamic_object_data::unknown);
irep_typedefs(dereference, dereference_data, esbmct::takestype,
              expr2tc, dereference_data, &dereference_data::value);
irep_typedefs(valid_object, object_ops, esbmct::notype,
              expr2tc, object_ops, &object_ops::value);
irep_typedefs(deallocated_obj, object_ops, esbmct::notype,
              expr2tc, object_ops, &object_ops::value);
irep_typedefs(dynamic_size, object_ops, esbmct::notype,
              expr2tc, object_ops, &object_ops::value);
irep_typedefs(sideeffect, sideeffect_data, esbmct::takestype,
              expr2tc, sideeffect_data, &sideeffect_data::operand,
              expr2tc, sideeffect_data, &sideeffect_data::size,
              std::vector<expr2tc>, sideeffect_data,
              &sideeffect_data::arguments,
              type2tc, sideeffect_data, &sideeffect_data::alloctype,
              sideeffect_data::allockind, sideeffect_data,
              &sideeffect_data::kind);
irep_typedefs(code_block, code_block_data, esbmct::notype,
              std::vector<expr2tc>, code_block_data,
              &code_block_data::operands);
irep_typedefs(code_assign, code_assign_data, esbmct::notype,
              expr2tc, code_assign_data, &code_assign_data::target,
              expr2tc, code_assign_data, &code_assign_data::source);
irep_typedefs(code_init, code_assign_data, esbmct::notype,
              expr2tc, code_assign_data, &code_assign_data::target,
              expr2tc, code_assign_data, &code_assign_data::source);
irep_typedefs(code_decl, code_decl_data, esbmct::takestype,
              irep_idt, code_decl_data, &code_decl_data::value);
irep_typedefs(code_printf, code_printf_data, esbmct::notype,
              std::vector<expr2tc>, code_printf_data,
              &code_printf_data::operands);
irep_typedefs(code_expression, code_expression_data, esbmct::notype,
              expr2tc, code_expression_data, &code_expression_data::operand);
irep_typedefs(code_return, code_expression_data, esbmct::notype,
              expr2tc, code_expression_data, &code_expression_data::operand);
irep_typedefs_empty(code_skip, expr2t, esbmct::notype);
irep_typedefs(code_free, code_expression_data, esbmct::notype,
              expr2tc, code_expression_data, &code_expression_data::operand);
irep_typedefs(code_goto, code_goto_data, esbmct::notype,
              irep_idt, code_goto_data, &code_goto_data::target);
irep_typedefs(object_descriptor, object_desc_data, esbmct::takestype,
              expr2tc, object_desc_data, &object_desc_data::object,
              expr2tc, object_desc_data, &object_desc_data::offset,
              unsigned int, object_desc_data, &object_desc_data::alignment);
irep_typedefs(code_function_call, code_funccall_data, esbmct::notype,
              expr2tc, code_funccall_data, &code_funccall_data::ret,
              expr2tc, code_funccall_data, &code_funccall_data::function,
              std::vector<expr2tc>, code_funccall_data,
              &code_funccall_data::operands);
irep_typedefs(code_comma, code_comma_data, esbmct::takestype,
              expr2tc, code_comma_data, &code_comma_data::side_1,
              expr2tc, code_comma_data, &code_comma_data::side_2);
irep_typedefs(invalid_pointer, invalid_pointer_ops, esbmct::notype,
              expr2tc, pointer_ops, &pointer_ops::ptr_obj);
irep_typedefs(buffer_size, buffer_size_data, esbmct::takestype,
              expr2tc, buffer_size_data, &buffer_size_data::value);
irep_typedefs(code_asm, code_asm_data, esbmct::notype,
              irep_idt, code_asm_data, &code_asm_data::value);
irep_typedefs(code_cpp_del_array, code_expression_data, esbmct::notype,
              expr2tc, code_expression_data, &code_expression_data::operand);
irep_typedefs(code_cpp_delete, code_expression_data, esbmct::notype,
              expr2tc, code_expression_data, &code_expression_data::operand);
irep_typedefs(code_cpp_catch, code_cpp_catch_data, esbmct::notype,
              std::vector<irep_idt>, code_cpp_catch_data,
              &code_cpp_catch_data::exception_list);
irep_typedefs(code_cpp_throw, code_cpp_throw_data, esbmct::notype,
              expr2tc, code_cpp_throw_data, &code_cpp_throw_data::operand,
              std::vector<irep_idt>, code_cpp_throw_data,
              &code_cpp_throw_data::exception_list);
irep_typedefs(code_cpp_throw_decl, code_cpp_throw_decl_data, esbmct::notype,
              std::vector<irep_idt>, code_cpp_throw_decl_data,
              &code_cpp_throw_decl_data::exception_list);
irep_typedefs(code_cpp_throw_decl_end, code_cpp_throw_decl_data, esbmct::notype,
              std::vector<irep_idt>, code_cpp_throw_decl_data,
              &code_cpp_throw_decl_data::exception_list);
irep_typedefs(isinf, arith_1op, esbmct::notype,
              expr2tc, arith_1op, &arith_1op::value);
irep_typedefs(isnormal, arith_1op, esbmct::notype,
              expr2tc, arith_1op, &arith_1op::value);
irep_typedefs(concat, bit_2ops, esbmct::takestype,
              expr2tc, bit_2ops, &bit_2ops::side_1,
              expr2tc, bit_2ops, &bit_2ops::side_2);

/** Constant integer class.
 *  Records a constant integer of an arbitary precision, signed or unsigned.
 *  Simplification operations will cause the integer to be clipped to whatever
 *  bit size is in expr type.
 *  @extends constant_int_data
 */
class constant_int2t : public constant_int_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this integer.
   *  @param input BigInt object containing the integer we're dealing with
   */
  constant_int2t(const type2tc &type, const BigInt &input)
    : constant_int_expr_methods(type, constant_int_id, input) { }
  constant_int2t(const constant_int2t &ref)
    : constant_int_expr_methods(ref) { }

  /** Accessor for fetching machine-word unsigned integer of this constant */
  unsigned long as_ulong(void) const;
  /** Accessor for fetching machine-word integer of this constant */
  long as_long(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant fixedbv class. Records a fixed-width number in what I assume
 *  to be mantissa/exponent form, but which is described throughout CBMC code
 *  as fraction/integer parts. Stored in a fixedbvt.
 *  @extends constant_fixedbv_data
 */
class constant_fixedbv2t : public constant_fixedbv_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expression.
   *  @param value fixedbvt object containing number we'll be operating on
   */
  constant_fixedbv2t(const type2tc &type, const fixedbvt &value)
    : constant_fixedbv_expr_methods(type, constant_fixedbv_id, value) { }
  constant_fixedbv2t(const constant_fixedbv2t &ref)
    : constant_fixedbv_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant boolean value.
 *  Contains a constant bool; rather self explanatory.
 *  @extends constant_bool_data
 */
class constant_bool2t : public constant_bool_expr_methods
{
public:
  /** Primary constructor. @param value True or false */
  constant_bool2t(bool value)
    : constant_bool_expr_methods(type_pool.get_bool(), constant_bool_id, value)
      { }
  constant_bool2t(const constant_bool2t &ref)
    : constant_bool_expr_methods(ref) { }

  /** Return whether contained boolean is true. */
  bool is_true(void) const;
  /** Return whether contained boolean is false. */
  bool is_false(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant class for string constants.
 *  Contains an irep_idt representing the constant string.
 *  @extends constant_string_data
 */
class constant_string2t : public constant_string_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this string; presumably a string_type2t.
   *  @param stringref String pool'd string we're dealing with
   */
  constant_string2t(const type2tc &type, const irep_idt &stringref)
    : constant_string_expr_methods(type, constant_string_id, stringref) { }
  constant_string2t(const constant_string2t &ref)
    : constant_string_expr_methods(ref) { }

  /** Convert string to a constant length array of characters */
  expr2tc to_array(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant structure.
 *  Contains a vector of expressions containing each member of the struct
 *  we're dealing with, corresponding to the types and field names in the
 *  struct_type2t type.
 *  @extends constant_datatype_data
 */
class constant_struct2t : public constant_struct_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this structure, presumably a struct_type2t
   *  @param membrs Vector of member values that make up this struct.
   */
  constant_struct2t(const type2tc &type, const std::vector<expr2tc> &members)
    : constant_struct_expr_methods (type, constant_struct_id, members) { }
  constant_struct2t(const constant_struct2t &ref)
    : constant_struct_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant union expression.
 *  Almost the same as constant_struct2t - a vector of members corresponding
 *  to the members described in the type. However, it seems the values pumped
 *  at us by CBMC only ever have one member (at position 0) representing the
 *  most recent value written to the union.
 *  @extend constant_datatype_data
 */
class constant_union2t : public constant_union_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this structure, presumably a union_type2t
   *  @param membrs Vector of member values that make up this union.
   */
  constant_union2t(const type2tc &type, const std::vector<expr2tc> &members)
    : constant_union_expr_methods (type, constant_union_id, members) { }
  constant_union2t(const constant_union2t &ref)
    : constant_union_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant array.
 *  Contains a vector of array elements, pretty self explanatory. Only valid if
 *  its type has a constant sized array, can't have constant arrays of dynamic
 *  or infinitely sized arrays.
 *  @extends constant_datatype_data
 */
class constant_array2t : public constant_array_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this array, must be a constant sized array
   *  @param membrs Vector of elements in this array
   */
  constant_array2t(const type2tc &type, const std::vector<expr2tc> &members)
    : constant_array_expr_methods(type, constant_array_id, members) { }
  constant_array2t(const constant_array2t &ref)
    : constant_array_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

/** Constant array of one particular value.
 *  Expression with array type, possibly dynamic or infinitely sized, with
 *  all elements initialized to a single value.
 *  @extends constant_array_of_data
 */
class constant_array_of2t : public constant_array_of_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expression, must be an array.
   *  @param init Initializer for each element in this array
   */
  constant_array_of2t(const type2tc &type, const expr2tc &init)
    : constant_array_of_expr_methods(type, constant_array_of_id, init) { }
  constant_array_of2t(const constant_array_of2t &ref)
    : constant_array_of_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

/** Symbol type.
 *  Contains the name of some variable. Various levels of renaming.
 *  @extends symbol_data
 */
class symbol2t : public symbol_expr_methods
{
public:
  /** Primary constructor
   *  @param type Type that this symbol has
   *  @param init Name of this symbol
   */

  symbol2t(const type2tc &type, const irep_idt &init,
           renaming_level lev = level0, unsigned int l1 = 0,
           unsigned int l2 = 0, unsigned int trd = 0, unsigned int node = 0)
    : symbol_expr_methods(type, symbol_id, init, lev, l1, l2, trd, node) { }

  symbol2t(const symbol2t &ref)
    : symbol_expr_methods(ref){}

  static std::string field_names[esbmct::num_type_fields];
};

/** Typecast expression.
 *  Represents cast from contained expression 'from' to the type of this
 *  typecast.
 *  @extends typecast_data
 */
class typecast2t : public typecast_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type to typecast to
   *  @param from Expression to cast from.
   */
  typecast2t(const type2tc &type, const expr2tc &from)
    : typecast_expr_methods(type, typecast_id, from) { }
  typecast2t(const typecast2t &ref)
    : typecast_expr_methods(ref){}
  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** If-then-else expression.
 *  Represents a ternary operation, (cond) ? truevalue : falsevalue.
 *  @extends if_data
 */
class if2t : public if_expr_methods
{
public:
  /** Primary constructor
   *  @param type Type this expression evaluates to.
   *  @param cond Condition to evaulate which side of ternary operator is used.
   *  @param trueval Value to use if cond evaluates to true.
   *  @param falseval Value to use if cond evaluates to false.
   */
  if2t(const type2tc &type, const expr2tc &cond, const expr2tc &trueval,
       const expr2tc &falseval)
    : if_expr_methods(type, if_id, cond, trueval, falseval) {}
  if2t(const if2t &ref)
    : if_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Equality expression. Evaluate whether two exprs are the same. Always has
 *  boolean type. @extends relation_data */
class equality2t : public equality_expr_methods
{
public:
  equality2t(const expr2tc &v1, const expr2tc &v2)
    : equality_expr_methods(type_pool.get_bool(), equality_id, v1, v2) {}
  equality2t(const equality2t &ref)
    : equality_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Inequality expression. Evaluate whether two exprs are different. Always has
 *  boolean type. @extends relation_data */
class notequal2t : public notequal_expr_methods
{
public:
  notequal2t(const expr2tc &v1, const expr2tc &v2)
    : notequal_expr_methods(type_pool.get_bool(), notequal_id, v1, v2) {}
  notequal2t(const notequal2t &ref)
    : notequal_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Lessthan relation. Evaluate whether expression is less than another. Always
 *  has boolean type. @extends relation_data */
class lessthan2t : public lessthan_expr_methods
{
public:
  lessthan2t(const expr2tc &v1, const expr2tc &v2)
    : lessthan_expr_methods(type_pool.get_bool(), lessthan_id, v1, v2) {}
  lessthan2t(const lessthan2t &ref)
    : lessthan_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Greaterthan relation. Evaluate whether expression is greater than another.
 * Always has boolean type. @extends relation_data */
class greaterthan2t : public greaterthan_expr_methods
{
public:
  greaterthan2t(const expr2tc &v1, const expr2tc &v2)
    : greaterthan_expr_methods(type_pool.get_bool(), greaterthan_id, v1, v2) {}
  greaterthan2t(const greaterthan2t &ref)
    : greaterthan_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Lessthanequal relation. Evaluate whether expression is less-than or
 * equal to another. Always has boolean type. @extends relation_data */
class lessthanequal2t : public lessthanequal_expr_methods
{
public:
  lessthanequal2t(const expr2tc &v1, const expr2tc &v2)
  : lessthanequal_expr_methods(type_pool.get_bool(), lessthanequal_id, v1, v2){}
  lessthanequal2t(const lessthanequal2t &ref)
  : lessthanequal_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Greaterthanequal relation. Evaluate whether expression is greater-than or
 * equal to another. Always has boolean type. @extends relation_data */
class greaterthanequal2t : public greaterthanequal_expr_methods
{
public:
  greaterthanequal2t(const expr2tc &v1, const expr2tc &v2)
    : greaterthanequal_expr_methods(type_pool.get_bool(), greaterthanequal_id,
                                    v1, v2) {}
  greaterthanequal2t(const greaterthanequal2t &ref)
    : greaterthanequal_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Not operation. Inverts boolean operand. Always has boolean type.
 *  @extends not_data */
class not2t : public not_expr_methods
{
public:
  /** Primary constructor. @param val Boolean typed operand to invert. */
  not2t(const expr2tc &val)
  : not_expr_methods(type_pool.get_bool(), not_id, val) {}
  not2t(const not2t &ref)
  : not_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** And operation. Computes boolean value of (side_1 & side_2). Always results
 *  in boolean type. @extends logic_2ops */
class and2t : public and_expr_methods
{
public:
  /** Primary constructor. @param s1 Operand 1. @param s2 Operand 2. */
  and2t(const expr2tc &s1, const expr2tc &s2)
  : and_expr_methods(type_pool.get_bool(), and_id, s1, s2) {}
  and2t(const and2t &ref)
  : and_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Or operation. Computes boolean value of (side_1 | side_2). Always results
 *  in boolean type. @extends logic_2ops */
class or2t : public or_expr_methods
{
public:
  /** Primary constructor. @param s1 Operand 1. @param s2 Operand 2. */
  or2t(const expr2tc &s1, const expr2tc &s2)
  : or_expr_methods(type_pool.get_bool(), or_id, s1, s2) {}
  or2t(const or2t &ref)
  : or_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Xor operation. Computes boolean value of (side_1 ^ side_2). Always results
 *  in boolean type. @extends logic_2ops */
class xor2t : public xor_expr_methods
{
public:
  /** Primary constructor. @param s1 Operand 1. @param s2 Operand 2. */
  xor2t(const expr2tc &s1, const expr2tc &s2)
  : xor_expr_methods(type_pool.get_bool(), xor_id, s1, s2) {}
  xor2t(const xor2t &ref)
  : xor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Implies operation. Computes boolean value of (side_1 -> side_2). Always
 *  results in boolean type. @extends logic_2ops */
class implies2t : public implies_expr_methods
{
public:
  /** Primary constructor. @param s1 Operand 1. @param s2 Operand 2. */
  implies2t(const expr2tc &s1, const expr2tc &s2)
  : implies_expr_methods(type_pool.get_bool(), implies_id, s1, s2) {}
  implies2t(const implies2t &ref)
  : implies_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Bit and operation. Perform bit and between two bitvector operands. Types of
 *  this expr and both operands must match. @extends bit_2ops */
class bitand2t : public bitand_expr_methods
{
public:
  /** Primary constructor.
   *  @param t Type of this expr.
   *  @param s1 Operand 1.
   *  @param s2 Operand 2. */
  bitand2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitand_expr_methods(t, bitand_id, s1, s2) {}
  bitand2t(const bitand2t &ref)
  : bitand_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Bit or operation. Perform bit or between two bitvector operands. Types of
 *  this expr and both operands must match. @extends bit_2ops */
class bitor2t : public bitor_expr_methods
{
public:
  /** Primary constructor.
   *  @param t Type of this expr.
   *  @param s1 Operand 1.
   *  @param s2 Operand 2. */
  bitor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitor_expr_methods(t, bitor_id, s1, s2) {}
  bitor2t(const bitor2t &ref)
  : bitor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Bit xor operation. Perform bit xor between two bitvector operands. Types of
 *  this expr and both operands must match. @extends bit_2ops */
class bitxor2t : public bitxor_expr_methods
{
public:
  /** Primary constructor.
   *  @param t Type of this expr.
   *  @param s1 Operand 1.
   *  @param s2 Operand 2. */
  bitxor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitxor_expr_methods(t, bitxor_id, s1, s2) {}
  bitxor2t(const bitxor2t &ref)
  : bitxor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Bit nand operation. Perform bit nand between two bitvector operands. Types of
 *  this expr and both operands must match. @extends bit_2ops */
class bitnand2t : public bitnand_expr_methods
{
public:
  /** Primary constructor.
   *  @param t Type of this expr.
   *  @param s1 Operand 1.
   *  @param s2 Operand 2. */
  bitnand2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitnand_expr_methods(t, bitnand_id, s1, s2) {}
  bitnand2t(const bitnand2t &ref)
  : bitnand_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Bit nor operation. Perform bit nor between two bitvector operands. Types of
 *  this expr and both operands must match. @extends bit_2ops */
class bitnor2t : public bitnor_expr_methods
{
public:
  /** Primary constructor.
   *  @param t Type of this expr.
   *  @param s1 Operand 1.
   *  @param s2 Operand 2. */
  bitnor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitnor_expr_methods(t, bitnor_id, s1, s2) {}
  bitnor2t(const bitnor2t &ref)
  : bitnor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Bit nxor operation. Perform bit nxor between two bitvector operands. Types of
 *  this expr and both operands must match. @extends bit_2ops */
class bitnxor2t : public bitnxor_expr_methods
{
public:
  /** Primary constructor.
   *  @param t Type of this expr.
   *  @param s1 Operand 1.
   *  @param s2 Operand 2. */
  bitnxor2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : bitnxor_expr_methods(t, bitnxor_id, s1, s2) {}
  bitnxor2t(const bitnxor2t &ref)
  : bitnxor_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Bit nxor operation. Invert bits in bitvector operand. Operand must have the
 *  same type as this expr. @extends bitnot_data */
class bitnot2t : public bitnot_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expr.
   *  @param v Value to invert */
  bitnot2t(const type2tc &type, const expr2tc &v)
    : bitnot_expr_methods(type, bitnot_id, v) {}
  bitnot2t(const bitnot2t &ref)
    : bitnot_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Logical shift right. Shifts operand 1 to the right by the number of bits in
 *  operand 2, with zeros shifted into empty spaces. All types must be integers,
 *  will probably find that the shifted value type must match the expr type.
 *  @extends bit_2ops */
class lshr2t : public lshr_expr_methods
{
public:
  /** Primary constructor.
   *  @param t Type of this expression.
   *  @param s1 Value to be shifted.
   *  @param s2 Number of bits to shift by, potentially nondeterministic. */
  lshr2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
  : lshr_expr_methods(t, lshr_id, s1, s2) {}
  lshr2t(const lshr2t &ref)
  : lshr_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Arithmetic negation. Negate the operand, which must be a number type. Operand
 *  type must match expr type. @extends arith_1op */
class neg2t : public neg_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expr.
   *  @param val Value to negate. */
  neg2t(const type2tc &type, const expr2tc &val)
    : neg_expr_methods(type, neg_id, val) {}
  neg2t(const neg2t &ref)
    : neg_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Arithmetic abs. Take absolute value of the operand, which must be a number
 *  type. Operand type must match expr type. @extends arith_1op */
class abs2t : public abs_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expr.
   *  @param val Value to abs. */
  abs2t(const type2tc &type, const expr2tc &val)
    : abs_expr_methods(type, abs_id, val) {}
  abs2t(const abs2t &ref)
    : abs_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Addition operation. Adds two operands together. Must both be numeric types.
 *  Types of both operands and expr type should match. @extends arith_2ops */
class add2t : public add_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expr.
   *  @param v1 First operand.
   *  @param v2 Second operand. */
  add2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : add_expr_methods(type, add_id, v1, v2) {}
  add2t(const add2t &ref)
    : add_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Subtraction operation. Subtracts second operand from first operand. Must both
 *  be numeric types. Types of both operands and expr type should match.
 *  @extends arith_2ops */
class sub2t : public sub_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expr.
   *  @param v1 First operand.
   *  @param v2 Second operand. */
  sub2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : sub_expr_methods(type, sub_id, v1, v2) {}
  sub2t(const sub2t &ref)
    : sub_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Multiplication operation. Multiplies the two operands. Must both be numeric
 *  types. Types of both operands and expr type should match.
 *  @extends arith_2ops */
class mul2t : public mul_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expr.
   *  @param v1 First operand.
   *  @param v2 Second operand. */
  mul2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : mul_expr_methods(type, mul_id, v1, v2) {}
  mul2t(const mul2t &ref)
    : mul_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Division operation. Divides first operand by second operand. Must both be
 *  numeric types. Types of both operands and expr type should match.
 *  @extends arith_2ops */
class div2t : public div_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expr.
   *  @param v1 First operand.
   *  @param v2 Second operand. */
  div2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : div_expr_methods(type, div_id, v1, v2) {}
  div2t(const div2t &ref)
    : div_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Modulus operation. Takes modulus of first operand divided by 2nd operand.
 *  Should both be integer types. Types of both operands and expr type should
 *  match. @extends arith_2ops */
class modulus2t : public modulus_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expr.
   *  @param v1 First operand.
   *  @param v2 Second operand. */
  modulus2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : modulus_expr_methods(type, modulus_id, v1, v2) {}
  modulus2t(const modulus2t &ref)
    : modulus_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Shift left operation. Shifts contents of first operand left by number of
 *  bit positions indicated by the second operand. Both must be integers. Types
 *  of both operands and expr type should match. @extends arith_2ops */
class shl2t : public shl_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expr.
   *  @param v1 Value to shift.
   *  @param v2 Number of bits to to shift by. */
  shl2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : shl_expr_methods(type, shl_id, v1, v2) {}
  shl2t(const shl2t &ref)
    : shl_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Arithmetic Shift right operation. Shifts contents of first operand right by
 *  number of bit positions indicated by the second operand, preserving sign of
 *  original number. Both must be integers. Types of both operands and expr type
 *  should match. @extends arith_2ops */
class ashr2t : public ashr_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expr.
   *  @param v1 Value to shift.
   *  @param v2 Number of bits to to shift by. */
  ashr2t(const type2tc &type, const expr2tc &v1, const expr2tc &v2)
    : ashr_expr_methods(type, ashr_id, v1, v2) {}
  ashr2t(const ashr2t &ref)
    : ashr_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Same-object operation. Checks whether two operands with pointer type have the
 *  same pointer object or not. Always has boolean result.
 *  @extends same_object_data */
class same_object2t : public same_object_expr_methods
{
public:
  /** Primary constructor. @param v1 First object. @param v2 Second object. */
  same_object2t(const expr2tc &v1, const expr2tc &v2)
    : same_object_expr_methods(type_pool.get_bool(), same_object_id, v1, v2) {}
  same_object2t(const same_object2t &ref)
    : same_object_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Extract pointer offset. From an expression of pointer type, produce the
 *  number of bytes difference between where this pointer points to and the start
 *  of the object it points at. @extends pointer_ops */
class pointer_offset2t : public pointer_offset_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Model basic integer type.
   *  @param ptrobj Pointer object to get offset from. */
  pointer_offset2t(const type2tc &type, const expr2tc &ptrobj)
    : pointer_offset_expr_methods(type, pointer_offset_id, ptrobj) {}
  pointer_offset2t(const pointer_offset2t &ref)
    : pointer_offset_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Extract pointer object. From an expression of pointer type, produce the
 *  pointer object that this pointer points into. @extends pointer_ops */
class pointer_object2t : public pointer_object_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Model basic integer type.
   *  @param ptrobj Pointer object to get object from. */
  pointer_object2t(const type2tc &type, const expr2tc &ptrobj)
    : pointer_object_expr_methods(type, pointer_object_id, ptrobj) {}
  pointer_object2t(const pointer_object2t &ref)
    : pointer_object_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Address of operation. Takes some object as an argument - ideally a symbol
 *  renamed to level 1, unfortunately some string constants reach here. Produces
 *  pointer typed expression.
 *  @extends pointer_ops */
class address_of2t : public address_of_expr_methods
{
public:
  /** Primary constructor.
   *  @param subtype Subtype of pointer to generate. Crucially, the type of the
   *         expr is a pointer to this subtype. This is slightly unintuitive,
   *         might be changed in the future.
   *  @param ptrobj Item to take pointer to. */
  address_of2t(const type2tc &subtype, const expr2tc &ptrobj)
    : address_of_expr_methods(type2tc(new pointer_type2t(subtype)),
                              address_of_id, ptrobj) {}
  address_of2t(const address_of2t &ref)
    : address_of_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Extract byte from data. From a particular data structure, extracts a single
 *  byte from its byte representation, at a particular offset into the data
 *  structure. Must only evaluate to byte types.
 *  @extends byte_extract_data */
class byte_extract2t : public byte_extract_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expression. May only ever be an 8 bit integer
   *  @param is_big_endian Whether or not to use big endian byte representation
   *         of source object.
   *  @param source Object to extract data from. Any type.
   *  @param offset Offset into source data object to extract from. */
  byte_extract2t(const type2tc &type, const expr2tc &source,
                 const expr2tc &offset, bool is_big_endian)
    : byte_extract_expr_methods(type, byte_extract_id,
                               source, offset, is_big_endian) {}
  byte_extract2t(const byte_extract2t &ref)
    : byte_extract_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Update byte. Takes a data object and updates the value of a particular
 *  byte in its byte representation, at a particular offset into the data object.
 *  Output of expression is a new copy of the source object, with the updated
 *  value. @extends byte_update_data */
class byte_update2t : public byte_update_expr_methods
{
public:
  /** Primary constructor
   *  @param type Type of resulting, updated, data object.
   *  @param is_big_endian Whether to use big endian byte representation.
   *  @param source Source object in which to update a byte.
   *  @param updateval Value of byte to  update source with. */
  byte_update2t(const type2tc &type, const expr2tc &source,
                 const expr2tc &offset, const expr2tc &updateval,
                 bool is_big_endian)
    : byte_update_expr_methods(type, byte_update_id, source, offset,
                               updateval, is_big_endian) {}
  byte_update2t(const byte_update2t &ref)
    : byte_update_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** With operation. Updates either an array or a struct/union with a new element
 *  or member. Expression value is the arary or struct/union with the updated
 *  value. Ideally in the future this will become two operations, one for arrays
 *  and one for structs/unions. @extends with_data */
class with2t : public with_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of this expression; Same as source.
   *  @param source Data object to update.
   *  @param field Field to update - a constant string naming the field if source
   *         is a struct/union, or an integer index if source is an array. */
  with2t(const type2tc &type, const expr2tc &source, const expr2tc &field,
         const expr2tc &value)
    : with_expr_methods(type, with_id, source, field, value) {}
  with2t(const with2t &ref)
    : with_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Member operation. Extracts a particular member out of a struct or union.
 *  @extends member_data */
class member2t : public member_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of extracted member.
   *  @param source Data structure to extract from.
   *  @param memb ΩName of member to extract.  */
  member2t(const type2tc &type, const expr2tc &source, const irep_idt &memb)
    : member_expr_methods(type, member_id, source, memb) {}
  member2t(const member2t &ref)
    : member_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Array index operation. Extracts an element from an array at a particular
 *  index. @extends index_data */
class index2t : public index_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of element extracted.
   *  @param source Array to extract data from.
   *  @param index Element in source to extract from. */
  index2t(const type2tc &type, const expr2tc &source, const expr2tc &index)
    : index_expr_methods(type, index_id, source, index) {}
  index2t(const index2t &ref)
    : index_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Is string zero operation. Checks to see whether string operand is zero or
 *  not. This is something string-abstraction related.
 *  Boolean result. @extends string_ops */
class zero_string2t : public zero_string_expr_methods
{
public:
  /** Primary constructor. @param string String type operand to test. */
  zero_string2t(const expr2tc &string)
    : zero_string_expr_methods(type_pool.get_bool(), zero_string_id, string) {}
  zero_string2t(const zero_string2t &ref)
    : zero_string_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Check for zero length string. Boolean result. @extends string_ops */
class zero_length_string2t : public zero_length_string_expr_methods
{
public:
  /** Primary constructor. @param string String type operand to test. */
  zero_length_string2t(const expr2tc &string)
    : zero_length_string_expr_methods(type_pool.get_bool(),
                                      zero_length_string_id, string) {}
  zero_length_string2t(const zero_length_string2t &ref)
    : zero_length_string_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Is operand not-a-number. Used to implement C library isnan function for
 *  float/double values. Boolean result. @extends isnan_data */
class isnan2t : public isnan_expr_methods
{
public:
  /** Primary constructor. @param value Number value to test for nan */
  isnan2t(const expr2tc &value)
    : isnan_expr_methods(type_pool.get_bool(), isnan_id, value) {}
  isnan2t(const isnan2t &ref)
    : isnan_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Check whether operand overflows. Operand must be either add, subtract,
 *  or multiply, and have integer operands themselves. If the result of the
 *  operation doesn't fit in the bitwidth of the operands, this expr evaluates
 *  to true. XXXjmorse - in the future we should ensure the type of the
 *  operand is the expected type result of the operation. That way we can tell
 *  whether to do a signed or unsigned over/underflow test.
 *  @extends overflow_ops */
class overflow2t : public overflow_expr_methods
{
public:
  /** Primary constructor.
   *  @param operand Operation to test overflow on; either an add, subtract, or
   *         multiply. */
  overflow2t(const expr2tc &operand)
    : overflow_expr_methods(type_pool.get_bool(), overflow_id, operand) {}
  overflow2t(const overflow2t &ref)
    : overflow_expr_methods(ref) {}

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

/** Test if a cast overflows. Check to see whether casting the operand to a
 *  particular bitsize will cause an integer overflow. If it does, this expr
 *  evaluates to true. @extends overflow_cast_data */
class overflow_cast2t : public overflow_cast_expr_methods
{
public:
  /** Primary constructor.
   *  @param operand Value to test cast out on. Should have integer type.
   *  @param bits Number of integer bits to cast operand to.  */
  overflow_cast2t(const expr2tc &operand, unsigned int bits)
    : overflow_cast_expr_methods(type_pool.get_bool(), overflow_cast_id,
                                 operand, bits) {}
  overflow_cast2t(const overflow_cast2t &ref)
    : overflow_cast_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Test for negation overflows. Check whether or not negating an operand would
 *  lead to an integer overflow - for example, there's no representation of
 *  -INT_MIN. Evaluates to true if overflow would occur. @extends overflow_ops */
class overflow_neg2t : public overflow_neg_expr_methods
{
public:
  /** Primary constructor. @param operand Integer to test negation of. */
  overflow_neg2t(const expr2tc &operand)
    : overflow_neg_expr_methods(type_pool.get_bool(), overflow_neg_id,
                                operand) {}
  overflow_neg2t(const overflow_neg2t &ref)
    : overflow_neg_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Record unknown data value. Exclusively for use in pointer analysis to record
 *  the fact that we point at an unknown item of data. @extends expr2t */
class unknown2t : public unknown_expr_methods
{
public:
  /** Primary constructor. @param type Type of unknown data item */
  unknown2t(const type2tc &type)
    : unknown_expr_methods(type, unknown_id) {}
  unknown2t(const unknown2t &ref)
    : unknown_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Record invalid data value. Exclusively for use in pointer analysis to record
 *  the fact that what we point at is guarenteed to be invalid or nonexistant.
 *  @extends expr2t */
class invalid2t : public invalid_expr_methods
{
public:
  invalid2t(const type2tc &type)
    : invalid_expr_methods(type, invalid_id) {}
  invalid2t(const invalid2t &ref)
    : invalid_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Record null pointer value. Exclusively for use in pointer analysis to record
 *  the fact that a pointer can be NULL. @extends expr2t */
class null_object2t : public null_object_expr_methods
{
public:
  null_object2t(const type2tc &type)
    : null_object_expr_methods(type, null_object_id) {}
  null_object2t(const null_object2t &ref)
    : null_object_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Record a dynamicly allocated object. Exclusively for use in pointer analysis.
 *  @extends dynamic_object_data */
class dynamic_object2t : public dynamic_object_expr_methods
{
public:
  dynamic_object2t(const type2tc &type, const expr2tc inst,
                   bool inv, bool uknown)
    : dynamic_object_expr_methods(type, dynamic_object_id, inst, inv, uknown) {}
  dynamic_object2t(const dynamic_object2t &ref)
    : dynamic_object_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Dereference operation. Expanded by symbolic execution into an if-then-else
 *  set of cases that take the value set of what this pointer might point at,
 *  examines the pointer's pointer object, and constructs a huge if-then-else
 *  case to evaluate to the appropriate data object for this pointer.
 *  @extends dereference_data */
class dereference2t : public dereference_expr_methods
{
public:
  /** Primary constructor.
   *  @param type Type of dereferenced data.
   *  @param operand Pointer to dereference. */
  dereference2t(const type2tc &type, const expr2tc &operand)
    : dereference_expr_methods(type, dereference_id, operand) {}
  dereference2t(const dereference2t &ref)
    : dereference_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Test whether ptr is valid. Expanded at symex time to look up whether or not
 *  the pointer operand is invalid (i.e., doesn't point at something and thus
 *  would be invalid to dereference). Boolean result. @extends object_ops */
class valid_object2t : public valid_object_expr_methods
{
public:
  /** Primary constructor. @param operand Pointer value to examine for validity*/
  valid_object2t(const expr2tc &operand)
    : valid_object_expr_methods(type_pool.get_bool(), valid_object_id, operand)
      {}
  valid_object2t(const valid_object2t &ref)
    : valid_object_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Test pointer for deallocation. Check for use after free: this irep is
 *  expanded at symex time to look up whether or not the operand is a) an invalid
 *  object, and b) if it is, whether it's been marked as being deallocated.
 *  Evalutes to true if that's the case. @extends object_ops */
class deallocated_obj2t : public deallocated_obj_expr_methods
{
public:
  /** Primary constructor. @param operand Pointer to check for deallocation */
  deallocated_obj2t(const expr2tc &operand)
    : deallocated_obj_expr_methods(type_pool.get_bool(), deallocated_obj_id,
                                   operand) {}
  deallocated_obj2t(const deallocated_obj2t &ref)
    : deallocated_obj_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Retrieve dynamic size of pointer obj. For a dynamically allocated pointer
 *  object, retrieves its potentially nondeterministic object size. Expanded at
 *  symex time to access a modelling array. Not sure what happens if you feed
 *  it a nondynamic pointer, it'll probably give you a free variable.
 *  @extends object_ops */
class dynamic_size2t : public dynamic_size_expr_methods
{
public:
  /** Primary constructor. @param operand Pointer object to fetch size for. */
  dynamic_size2t(const expr2tc &operand)
    : dynamic_size_expr_methods(type_pool.get_uint32(), dynamic_size_id,
        operand) {}
  dynamic_size2t(const dynamic_size2t &ref)
    : dynamic_size_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

/** Irep for various side effects. Stores data about various things that can
 *  cause side effects, such as memory allocations, nondeterministic value
 *  allocations (nondet_* funcs,).
 *
 *  Also allows for function-calls to be represented. This side-effect
 *  expression is how function calls inside expressions are represented during
 *  parsing, and are all flattened out prior to GOTO program creation. However,
 *  under certain circumstances irep2 needs to represent such function calls,
 *  so this facility is preserved in irep2.
 *
 *  @extends sideeffect_data */
class sideeffect2t : public sideeffect_expr_methods
{
public:
  /** Primary constructor.
   *  @param t Type this side-effect evaluates to.
   *  @param operand Not really certain. Sometimes turns up in string-irep.
   *  @param sz Size of dynamic allocation to make.
   *  @param alloct Type of piece of data to allocate.
   *  @param a Vector of arguments to function call. */
  sideeffect2t(const type2tc &t, const expr2tc &oper, const expr2tc &sz,
               const std::vector<expr2tc> &a,
               const type2tc &alloct, allockind k)
    : sideeffect_expr_methods(t, sideeffect_id, oper, sz, a, alloct, k) {}
  sideeffect2t(const sideeffect2t &ref)
    : sideeffect_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_block2t : public code_block_expr_methods
{
public:
  code_block2t(const std::vector<expr2tc> &operands)
    : code_block_expr_methods(type_pool.get_empty(), code_block_id, operands) {}
  code_block2t(const code_block2t &ref)
    : code_block_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_assign2t : public code_assign_expr_methods
{
public:
  code_assign2t(const expr2tc &target, const expr2tc &source)
    : code_assign_expr_methods(type_pool.get_empty(), code_assign_id,
                               target, source) {}
  code_assign2t(const code_assign2t &ref)
    : code_assign_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

// NB: code_init2t is a specialization of code_assign2t
class code_init2t : public code_init_expr_methods
{
public:
  code_init2t(const expr2tc &target, const expr2tc &source)
    : code_init_expr_methods(type_pool.get_empty(), code_init_id,
                               target, source) {}
  code_init2t(const code_init2t &ref)
    : code_init_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_decl2t : public code_decl_expr_methods
{
public:
  code_decl2t(const type2tc &t, const irep_idt &name)
    : code_decl_expr_methods(t, code_decl_id, name){}
  code_decl2t(const code_decl2t &ref)
    : code_decl_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_printf2t : public code_printf_expr_methods
{
public:
  code_printf2t(const std::vector<expr2tc> &opers)
    : code_printf_expr_methods(type_pool.get_empty(), code_printf_id, opers) {}
  code_printf2t(const code_printf2t &ref)
    : code_printf_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_expression2t : public code_expression_expr_methods
{
public:
  code_expression2t(const expr2tc &oper)
    : code_expression_expr_methods(type_pool.get_empty(), code_expression_id,
                                   oper) {}
  code_expression2t(const code_expression2t &ref)
    : code_expression_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_return2t : public code_return_expr_methods
{
public:
  code_return2t(const expr2tc &oper)
    : code_return_expr_methods(type_pool.get_empty(), code_return_id, oper) {}
  code_return2t(const code_return2t &ref)
    : code_return_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_skip2t : public code_skip_expr_methods
{
public:
  code_skip2t()
    : code_skip_expr_methods(type_pool.get_empty(), code_skip_id) {}
  code_skip2t(const code_skip2t &ref)
    : code_skip_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_free2t : public code_free_expr_methods
{
public:
  code_free2t(const expr2tc &oper)
    : code_free_expr_methods(type_pool.get_empty(), code_free_id, oper) {}
  code_free2t(const code_free2t &ref)
    : code_free_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class code_goto2t : public code_goto_expr_methods
{
public:
  code_goto2t(const irep_idt &targ)
    : code_goto_expr_methods(type_pool.get_empty(), code_goto_id, targ) {}
  code_goto2t(const code_goto2t &ref)
    : code_goto_expr_methods(ref) {}

  static std::string field_names[esbmct::num_type_fields];
};

class object_descriptor2t : public object_descriptor_expr_methods
{
public:
  object_descriptor2t(const type2tc &t, const expr2tc &root,const expr2tc &offs,
                      unsigned int alignment)
    : object_descriptor_expr_methods(t, object_descriptor_id, root, offs,
                                     alignment) {}
  object_descriptor2t(const object_descriptor2t &ref)
    : object_descriptor_expr_methods(ref) {}

  const expr2tc &get_root_object(void) const;

  static std::string field_names[esbmct::num_type_fields];
};

class code_function_call2t : public code_function_call_expr_methods
{
public:
  code_function_call2t(const expr2tc &r, const expr2tc &func,
                       const std::vector<expr2tc> args)
    : code_function_call_expr_methods(type_pool.get_empty(),
                                      code_function_call_id, r, func, args) {}
  code_function_call2t(const code_function_call2t &ref)
    : code_function_call_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_comma2t : public code_comma_expr_methods
{
public:
  code_comma2t(const type2tc &t, const expr2tc &s1, const expr2tc &s2)
    : code_comma_expr_methods(t, code_comma_id, s1, s2) {}
  code_comma2t(const code_comma2t &ref)
    : code_comma_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class invalid_pointer2t : public invalid_pointer_expr_methods
{
public:
  invalid_pointer2t(const expr2tc &obj)
    : invalid_pointer_expr_methods(type_pool.get_bool(), invalid_pointer_id,
                                   obj) {}
  invalid_pointer2t(const invalid_pointer2t &ref)
    : invalid_pointer_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class buffer_size2t : public buffer_size_expr_methods
{
public:
  buffer_size2t(const type2tc &t, const expr2tc &obj)
    : buffer_size_expr_methods(t, buffer_size_id, obj) {}
  buffer_size2t(const buffer_size2t &ref)
    : buffer_size_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_asm2t : public code_asm_expr_methods
{
public:
  code_asm2t(const type2tc &type, const irep_idt &stringref)
    : code_asm_expr_methods(type, code_asm_id, stringref) { }
  code_asm2t(const code_asm2t &ref)
    : code_asm_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_cpp_del_array2t : public code_cpp_del_array_expr_methods
{
public:
  code_cpp_del_array2t(const expr2tc &v)
    : code_cpp_del_array_expr_methods(type_pool.get_empty(),
                                      code_cpp_del_array_id, v) { }
  code_cpp_del_array2t(const code_cpp_del_array2t &ref)
    : code_cpp_del_array_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_cpp_delete2t : public code_cpp_delete_expr_methods
{
public:
  code_cpp_delete2t(const expr2tc &v)
    : code_cpp_delete_expr_methods(type_pool.get_empty(),
                                   code_cpp_delete_id, v) { }
  code_cpp_delete2t(const code_cpp_delete2t &ref)
    : code_cpp_delete_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_cpp_catch2t : public code_cpp_catch_expr_methods
{
public:
  code_cpp_catch2t(const std::vector<irep_idt> &el)
    : code_cpp_catch_expr_methods(type_pool.get_empty(),
                                   code_cpp_catch_id, el) { }
  code_cpp_catch2t(const code_cpp_catch2t &ref)
    : code_cpp_catch_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_cpp_throw2t : public code_cpp_throw_expr_methods
{
public:
  code_cpp_throw2t(const expr2tc &o, const std::vector<irep_idt> &l)
    : code_cpp_throw_expr_methods(type_pool.get_empty(), code_cpp_throw_id,
                                  o, l){}
  code_cpp_throw2t(const code_cpp_throw2t &ref)
    : code_cpp_throw_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_cpp_throw_decl2t : public code_cpp_throw_decl_expr_methods
{
public:
  code_cpp_throw_decl2t(const std::vector<irep_idt> &l)
    : code_cpp_throw_decl_expr_methods(type_pool.get_empty(),
                                       code_cpp_throw_decl_id, l){}
  code_cpp_throw_decl2t(const code_cpp_throw_decl2t &ref)
    : code_cpp_throw_decl_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class code_cpp_throw_decl_end2t : public code_cpp_throw_decl_end_expr_methods
{
public:
  code_cpp_throw_decl_end2t(const std::vector<irep_idt> &exl)
    : code_cpp_throw_decl_end_expr_methods(type_pool.get_empty(),
                                           code_cpp_throw_decl_end_id, exl) { }
  code_cpp_throw_decl_end2t(const code_cpp_throw_decl_end2t &ref)
    : code_cpp_throw_decl_end_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class isinf2t : public isinf_expr_methods
{
public:
  isinf2t(const expr2tc &val)
    : isinf_expr_methods(type_pool.get_bool(), isinf_id, val) { }
  isinf2t(const isinf2t &ref)
    : isinf_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class isnormal2t : public isnormal_expr_methods
{
public:
  isnormal2t(const expr2tc &val)
    : isnormal_expr_methods(type_pool.get_bool(), isnormal_id, val) { }
  isnormal2t(const isnormal2t &ref)
    : isnormal_expr_methods(ref) { }

  static std::string field_names[esbmct::num_type_fields];
};

class concat2t : public concat_expr_methods
{
public:
  concat2t(const type2tc &type, const expr2tc &forward, const expr2tc &aft)
    : concat_expr_methods(type, concat_id, forward, aft) { }
  concat2t(const concat2t &ref)
    : concat_expr_methods(ref) { }

  virtual expr2tc do_simplify(bool second) const;

  static std::string field_names[esbmct::num_type_fields];
};

inline bool operator==(std::shared_ptr<type2t> const & a, std::shared_ptr<type2t> const & b)
{
  return (*a.get() == *b.get());
}

inline bool operator!=(std::shared_ptr<type2t> const & a, std::shared_ptr<type2t> const & b)
{
  return !(a == b);
}

inline bool operator<(std::shared_ptr<type2t> const & a, std::shared_ptr<type2t> const & b)
{
  return (*a.get() < *b.get());
}

inline bool operator>(std::shared_ptr<type2t> const & a, std::shared_ptr<type2t> const & b)
{
  return (*b.get() < *a.get());
}

inline bool operator==(const expr2tc& a, const expr2tc& b)
{
  return (*a.get() == *b.get());
}

inline bool operator!=(const expr2tc& a, const expr2tc& b)
{
  return (*a.get() != *b.get());
}

inline bool operator<(const expr2tc& a, const expr2tc& b)
{
  return (*a.get() < *b.get());
}

inline bool operator>(const expr2tc& a, const expr2tc& b)
{
  return (*b.get() < *a.get());
}

inline std::ostream& operator<<(std::ostream &out, const expr2tc& a)
{
  out << a->pretty(0);
  return out;
}

struct irep2_hash
{
  size_t operator()(const expr2tc &ref) const { return ref.crc(); }
};

struct type2_hash
{
  size_t operator()(const type2tc &ref) const { return ref->crc(); }
};

// Same deal as for "type_macros".
#ifdef NDEBUG
#define dynamic_cast static_cast
#endif
#define expr_macros(name) \
  inline bool is_##name##2t(const expr2tc &t) \
    { return t->expr_id == expr2t::name##_id; } \
  inline bool is_##name##2t(const expr2t &r) \
    { return r.expr_id == expr2t::name##_id; } \
  inline const name##2t & to_##name##2t(const expr2tc &t) \
    { return dynamic_cast<const name##2t &> (*t); } \
  inline name##2t & to_##name##2t(expr2tc &t) \
    { return dynamic_cast<name##2t &> (*t.get()); }

expr_macros(constant_int);
expr_macros(constant_fixedbv);
expr_macros(constant_bool);
expr_macros(constant_string);
expr_macros(constant_struct);
expr_macros(constant_union);
expr_macros(constant_array);
expr_macros(constant_array_of);
expr_macros(symbol);
expr_macros(typecast);
expr_macros(if);
expr_macros(equality);
expr_macros(notequal);
expr_macros(lessthan);
expr_macros(greaterthan);
expr_macros(lessthanequal);
expr_macros(greaterthanequal);
expr_macros(not);
expr_macros(and);
expr_macros(or);
expr_macros(xor);
expr_macros(implies);
expr_macros(bitand);
expr_macros(bitor);
expr_macros(bitxor);
expr_macros(bitnand);
expr_macros(bitnor);
expr_macros(bitnxor);
expr_macros(bitnot);
expr_macros(lshr);
expr_macros(neg);
expr_macros(abs);
expr_macros(add);
expr_macros(sub);
expr_macros(mul);
expr_macros(div);
expr_macros(modulus);
expr_macros(shl);
expr_macros(ashr);
expr_macros(same_object);
expr_macros(pointer_offset);
expr_macros(pointer_object);
expr_macros(address_of);
expr_macros(byte_extract);
expr_macros(byte_update);
expr_macros(with);
expr_macros(member);
expr_macros(index);
expr_macros(zero_string);
expr_macros(zero_length_string);
expr_macros(isnan);
expr_macros(overflow);
expr_macros(overflow_cast);
expr_macros(overflow_neg);
expr_macros(unknown);
expr_macros(invalid);
expr_macros(null_object);
expr_macros(dynamic_object);
expr_macros(dereference);
expr_macros(valid_object);
expr_macros(deallocated_obj);
expr_macros(dynamic_size);
expr_macros(sideeffect);
expr_macros(code_block);
expr_macros(code_assign);
expr_macros(code_init);
expr_macros(code_decl);
expr_macros(code_printf);
expr_macros(code_expression);
expr_macros(code_return);
expr_macros(code_skip);
expr_macros(code_free);
expr_macros(code_goto);
expr_macros(object_descriptor);
expr_macros(code_function_call);
expr_macros(code_comma);
expr_macros(invalid_pointer);
expr_macros(buffer_size);
expr_macros(code_asm);
expr_macros(code_cpp_del_array);
expr_macros(code_cpp_delete);
expr_macros(code_cpp_catch);
expr_macros(code_cpp_throw);
expr_macros(code_cpp_throw_decl);
expr_macros(code_cpp_throw_decl_end);
expr_macros(isinf);
expr_macros(isnormal);
expr_macros(concat);
#undef expr_macros
#ifdef dynamic_cast
#undef dynamic_cast
#endif

inline bool is_constant_expr(const expr2tc &t)
{
  return t->expr_id == expr2t::constant_int_id ||
         t->expr_id == expr2t::constant_fixedbv_id ||
         t->expr_id == expr2t::constant_bool_id ||
         t->expr_id == expr2t::constant_string_id ||
         t->expr_id == expr2t::constant_struct_id ||
         t->expr_id == expr2t::constant_union_id ||
         t->expr_id == expr2t::constant_array_id ||
         t->expr_id == expr2t::constant_array_of_id;
}

inline bool is_structure_type(const type2tc &t)
{
  return t->type_id == type2t::struct_id || t->type_id == type2t::union_id;
}

inline bool is_structure_type(const expr2tc &e)
{
  return is_structure_type(e->type);
}

/** Test if expr is true. First checks whether the expr is a constant bool, and
 *  then whether it's true-valued. If these are both true, return true,
 *  otherwise return false.
 *  @param expr Expression to check for true value.
 *  @return Whether or not expr is true-valued.
 */
inline bool
is_true(const expr2tc &expr)
{
  if (is_constant_bool2t(expr) && to_constant_bool2t(expr).constant_value)
    return true;
  else
    return false;
}

/** Test if expr is false. First checks whether the expr is a constant bool, and
 *  then whether it's false-valued. If these are both true, return true,
 *  otherwise return false.
 *  @param expr Expression to check for false value.
 *  @return Whether or not expr is true-valued.
 */
inline bool
is_false(const expr2tc &expr)
{
  if (is_constant_bool2t(expr) && !to_constant_bool2t(expr).constant_value)
    return true;
  else
    return false;
}

// To initialize the below at a defined time...
void init_expr_constants(void);

extern const expr2tc true_expr;
extern const expr2tc false_expr;
extern const constant_int2tc zero_u32;
extern const constant_int2tc one_u32;
extern const constant_int2tc zero_32;
extern const constant_int2tc one_32;
extern const constant_int2tc zero_u64;
extern const constant_int2tc one_u64;
extern const constant_int2tc zero_64;
extern const constant_int2tc one_64;
extern const constant_int2tc zero_ulong;
extern const constant_int2tc one_ulong;
extern const constant_int2tc zero_long;
extern const constant_int2tc one_long;

inline expr2tc
gen_uint(const type2tc &type, unsigned long val)
{
  constant_int2tc v(type, BigInt(val));
  return v;
}

inline expr2tc
gen_ulong(unsigned long val)
{
  constant_int2tc v(type_pool.get_uint(config.ansi_c.word_size), BigInt(val));
  return v;
}

inline const type2tc &
get_uint8_type(void)
{
  return type_pool.get_uint8();
}

inline const type2tc &
get_uint16_type(void)
{
  return type_pool.get_uint16();
}

inline const type2tc &
get_uint32_type(void)
{
  return type_pool.get_uint32();
}

inline const type2tc &
get_uint64_type(void)
{
  return type_pool.get_uint64();
}

inline const type2tc &
get_int8_type(void)
{
  return type_pool.get_int8();
}

inline const type2tc &
get_int16_type(void)
{
  return type_pool.get_int16();
}

inline const type2tc &
get_int32_type(void)
{
  return type_pool.get_int32();
}

inline const type2tc &
get_int64_type(void)
{
  return type_pool.get_int64();
}

inline const type2tc &
get_uint_type(unsigned int sz)
{
  return type_pool.get_uint(sz);
}

inline const type2tc &
get_int_type(unsigned int sz)
{
  return type_pool.get_int(sz);
}

inline const type2tc &
get_bool_type(void)
{
  return type_pool.get_bool();
}

inline const type2tc &
get_empty_type(void)
{
  return type_pool.get_empty();
}

inline const type2tc &
get_pointer_type(const typet &val)
{
  return type_pool.get_pointer(val);
}

inline const type2tc &
get_array_subtype(const type2tc &type)
{
  return to_array_type(type).subtype;
}

inline const type2tc &
get_base_array_subtype(const type2tc &type)
{
  const auto &subtype = to_array_type(type).subtype;
  if (is_array_type(subtype))
    return get_base_array_subtype(subtype);
  else
    return subtype;
}

#endif /* _UTIL_IREP2_H_ */
