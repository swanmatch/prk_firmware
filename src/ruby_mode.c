#include <picorbc.h>
#include "ruby_mode.h"

#define NODE_BOX_SIZE 30

int loglevel = LOGLEVEL_INFO;

static ParserState *p;
static StreamInterface *si;

mrbc_tcb *tcb_sandbox = NULL; /* from ruby_mode.h */

void
c_sandbox_state(mrb_vm *vm, mrb_value *v, int argc)
{
  SET_INT_RETURN(tcb_sandbox->state);
}

void
c_sandbox_result(mrb_vm *vm, mrb_value *v, int argc)
{
  mrbc_vm *sandbox_vm = (mrbc_vm *)&tcb_sandbox->vm;
  if (sandbox_vm->error_code == 0) {
    SET_RETURN(sandbox_vm->regs[0]);
  } else {
    SET_RETURN( mrbc_string_new_cstr(vm, "Error: Runtime error") );
  }
  mrbc_suspend_task(tcb_sandbox);
  StreamInterface_free(si);
  Compiler_parserStateFree(p);
}

void
c_invoke_ruby(mrb_vm *vm, mrb_value *v, int argc)
{
  p = Compiler_parseInitState(NODE_BOX_SIZE);
  si = StreamInterface_new(GET_STRING_ARG(1), STREAM_TYPE_MEMORY);
  if (!Compiler_compile(p, si)) {
    SET_FALSE_RETURN();
    return;
  }
  mrbc_vm *sandbox_vm;
  if (!tcb_sandbox) {
    tcb_sandbox = mrbc_create_task(p->scope->vm_code, 0);
    sandbox_vm = (mrbc_vm *)&tcb_sandbox->vm;
  } else {
    sandbox_vm = (mrbc_vm *)&tcb_sandbox->vm;
    if(mrbc_load_mrb(sandbox_vm, p->scope->vm_code) != 0) {
      SET_FALSE_RETURN();
      return;
    }
    sandbox_vm->pc_irep = sandbox_vm->irep;
    sandbox_vm->inst = sandbox_vm->pc_irep->code;
    sandbox_vm->current_regs = sandbox_vm->regs;
    sandbox_vm->callinfo_tail = NULL;
    sandbox_vm->target_class = mrbc_class_object;
    sandbox_vm->exc = 0;
    sandbox_vm->error_code = 0;
    sandbox_vm->flag_preemption = 0;
    mrbc_resume_task(tcb_sandbox);
  }
  SET_TRUE_RETURN();
}
