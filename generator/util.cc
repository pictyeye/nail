#include "nailtool.h"
bool parameter_type_check(parameterlist *param, parameterdefinitionlist *def, const Scope &scope){
  if(param == NULL && def  == NULL)
    return true;
  if(param != NULL || def != NULL)
    return false;
  if(param->count != def->count)
    return false;
  for(int i =0; i<param->count; i++)
  {
    if(param->elem[i].N_type != def->elem[i].N_type)
      return false;
    switch(param->elem[i].N_type){
    case DSTREAM:
      if(mk_str(param->elem[i].pstream) != mk_str(def->elem[i].dstream))
        return false;
      break;
    case DDEPENDENCY:{
      std::string post;
      if(scope.dependency_type(mk_str(param->elem[i].pdependency)) != typedef_type(*def->elem[i].ddependency.type,"",&post) || post != "")
        return false;
      break;
    }
    }
  }
  return true;
}  
std::string int_type(const intp &intp ){
    int length = boost::lexical_cast<int>(mk_str(intp.sign));
    if(length<=8) length = 8;
    else if(length<=16) length=16;
    else if(length<=32) length=32;
    else if(length<=64) length=64;
    else throw std::range_error("Integer longer than 64 bits");
    return (boost::format("%s%d_t") % (intp.N_type == UNSIGN ? "uint":"int" )% length).str();
  }

  // Gets the type for a top-level parser (to be used in the typedef)
std::string typedef_type(const parser &p, std::string name, std::string *post){
    const parserinner &inner(p.N_type == PR ? p.pr : p.paren);
    switch(inner.N_type){
    case INTEGER:
      return int_type(inner.integer.parser);
    case WRAP:
      return typedef_type(*inner.wrap.parser,name,post);
    case OPTIONAL:
      return (boost::format("%s*") % typedef_type(*inner.optional,name,NULL)).str();
    case NAME:
      return mk_str(inner.name.name);
    case REF:
      return (boost::format("%s *") % mk_str(inner.name.name)).str();
    case FIXEDARRAY:
      if(post){
        *post = (boost::format("[%s]%s") % intconstant_value(inner.fixedarray.length) % *post).str(); 
        return typedef_type(*inner.fixedarray.inner,name,post);
      } 
      else
        return (boost::format("%s *") % typedef_type(*inner.fixedarray.inner,name,NULL)).str();
    case NUNION:
      /*   FOREACH(parser,inner.UNION){
           if(!type_equal(inner.UNION.elem[0],parser))
           throw std::runtime_error("Grammar invalid at UNION");
           } */
      return typedef_type(*inner.nunion.elem[0],name,post);
      //  return (boost::format("%s[%s]") %typedef_type(*inner.fixedarray.inner,name)% intconstant_value(inner.fixedarray.length)).str();
    case STRUCTURE:
    case ARRAY:
    case CHOICE:
      return (boost::format("struct %s") % name).str();
    case APPLY:
      return typedef_type(*inner.apply.inner,name, post);
    default:
      assert(0);
      return 0;
    }
  }
//Negative of a constraint. 
void constraint(std::ostream &out,std::string val, constraintelem &e){
  switch(e.N_type){
  case VALUE:
    out << val << "!="<< intconstant_value(e.value);
    break;
  case RANGE:
    out << "(";
    if(e.range.max){
      out << val << ">" << intconstant_value(*e.range.max);
    }
    else {
      out << "0";
    }
    out << "||";
    if(e.range.min){
      out << val << "<" << intconstant_value(*e.range.min);
    }
    else {
      out << "0";
    }
    out << ")";
    break;
  default:
    assert("!foo");
  }
}
void constraint(std::ostream &out,std::string val,  intconstraint &c){
  switch(c.N_type){
  case SINGLE:
    constraint(val,c.single);
    break;
  case SET:
    {
      int first = 0;
      FOREACH(allowed,c.set){
        if(first++ != 0)
          out << " && ";
        constraint(val,*allowed);
      }
    }
    break;
  case NEGATE:
    out << "!(";
    constraint(val,*c.negate);
    out << ")";
    break;
  }
}
