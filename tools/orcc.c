
#include <orc/orc.h>
#include <orc-test/orctest.h>
#include <orc/orcparse.h>

#include <stdio.h>
#include <stdlib.h>

static char * read_file (const char *filename);
void output_code (OrcProgram *p, FILE *output);
void output_code_header (OrcProgram *p, FILE *output);
void output_code_test (OrcProgram *p, FILE *output);
void output_code_backup (OrcProgram *p, FILE *output);
static void print_defines (FILE *output);

int
main (int argc, char *argv[])
{
  char *code;
  int n;
  int i;
  OrcProgram **programs;
  const char *filename = "test.orc";
  FILE *output;

  orc_init ();
  orc_test_init ();

  if (argc >= 2) {
    filename = argv[1];
  }
  code = read_file (filename);
  if (!code) {
    printf("orcc <file.orc>\n");
    exit(1);
  }

  n = orc_parse (code, &programs);

  output = fopen ("out.c", "w");

  fprintf(output, "\n");
  fprintf(output, "/* autogenerated from %s */\n", filename);
  fprintf(output, "\n");
  fprintf(output, "#include <orc/orc.h>\n");
  fprintf(output, "#include <stdio.h>\n");
  fprintf(output, "#include <stdlib.h>\n");
  fprintf(output, "\n");
  fprintf(output, "\n");
  print_defines (output);
  fprintf(output, "\n");
  for(i=0;i<n;i++){
    output_code (programs[i], output);
  }

  fclose (output);

  output = fopen ("out.h", "w");

  fprintf(output, "\n");
  fprintf(output, "/* autogenerated from %s */\n", filename);
  fprintf(output, "\n");
  //fprintf(output, "#include <orc/orc.h>\n");
  //fprintf(output, "\n");
  fprintf(output, "#ifndef _ORC_OUT_H_\n");
  fprintf(output, "#define _ORC_OUT_H_\n");
  fprintf(output, "\n");
  for(i=0;i<n;i++){
    output_code_header (programs[i], output);
  }
  fprintf(output, "\n");
  fprintf(output, "#endif\n");
  fprintf(output, "\n");

  fclose (output);

  /* test program */
  output = fopen ("test_out.c", "w");

  fprintf(output, "\n");
  fprintf(output, "/* autogenerated from %s */\n", filename);
  fprintf(output, "\n");
  fprintf(output, "#include <orc/orc.h>\n");
  fprintf(output, "#include <orc-test/orctest.h>\n");
  fprintf(output, "#include <stdio.h>\n");
  fprintf(output, "#include <stdlib.h>\n");
  fprintf(output, "\n");
  print_defines (output);
  fprintf(output, "\n");
  for(i=0;i<n;i++){
    fprintf(output, "/* %s */\n", programs[i]->name);
    output_code_backup (programs[i], output);
  }
  fprintf(output, "\n");
  fprintf(output, "int\n");
  fprintf(output, "main (int argc, char *argv[])\n");
  fprintf(output, "{\n");
  fprintf(output, "  int error = FALSE;\n");
  fprintf(output, "\n");
  fprintf(output, "  orc_test_init ();\n");
  fprintf(output, "\n");
  for(i=0;i<n;i++){
    output_code_test (programs[i], output);
  }
  fprintf(output, "\n");
  fprintf(output, "  if (error) {\n");
  fprintf(output, "    return 1;\n");
  fprintf(output, "  };\n");
  fprintf(output, "  return 0;\n");
  fprintf(output, "}\n");

  fclose (output);

  return 0;
}


static void
print_defines (FILE *output)
{
  fprintf(output,
    "#define ORC_CLAMP(x,a,b) ((x)<(a) ? (a) : ((x)>(b) ? (b) : (x)))\n"
    "#define ORC_ABS(a) ((a)<0 ? -(a) : (a))\n"
    "#define ORC_MIN(a,b) ((a)<(b) ? (a) : (b))\n"
    "#define ORC_MAX(a,b) ((a)>(b) ? (a) : (b))\n"
    "#define ORC_SB_MAX 127\n"
    "#define ORC_SB_MIN (-1-ORC_SB_MAX)\n"
    "#define ORC_UB_MAX 255\n"
    "#define ORC_UB_MIN 0\n"
    "#define ORC_SW_MAX 32767\n"
    "#define ORC_SW_MIN (-1-ORC_SW_MAX)\n"
    "#define ORC_UW_MAX 65535\n"
    "#define ORC_UW_MIN 0\n"
    "#define ORC_SL_MAX 2147483647\n"
    "#define ORC_SL_MIN (-1-ORC_SL_MAX)\n"
    "#define ORC_UL_MAX 4294967295U\n"
    "#define ORC_UL_MIN 0\n"
    "#define ORC_CLAMP_SB(x) ORC_CLAMP(x,ORC_SB_MIN,ORC_SB_MAX)\n"
    "#define ORC_CLAMP_UB(x) ORC_CLAMP(x,ORC_UB_MIN,ORC_UB_MAX)\n"
    "#define ORC_CLAMP_SW(x) ORC_CLAMP(x,ORC_SW_MIN,ORC_SW_MAX)\n"
    "#define ORC_CLAMP_UW(x) ORC_CLAMP(x,ORC_UW_MIN,ORC_UW_MAX)\n"
    "#define ORC_CLAMP_SL(x) ORC_CLAMP(x,ORC_SL_MIN,ORC_SL_MAX)\n"
    "#define ORC_CLAMP_UL(x) ORC_CLAMP(x,ORC_UL_MIN,ORC_UL_MAX)\n"
    "#define ORC_SWAP_W(x) ((((x)&0xff)<<8) | (((x)&0xff00)>>8))\n"
    "#define ORC_SWAP_L(x) ((((x)&0xff)<<24) | (((x)&0xff00)<<8) | (((x)&0xff0000)>>8) | (((x)&0xff000000)>>24))\n"
    "#define ORC_PTR_OFFSET(ptr,offset) ((void *)(((unsigned char *)(ptr)) + (offset)))\n");
}

static char *
read_file (const char *filename)
{
  FILE *file = NULL;
  char *contents = NULL;
  long size;
  int ret;

  file = fopen (filename, "r");
  if (file == NULL) return NULL;

  ret = fseek (file, 0, SEEK_END);
  if (ret < 0) goto bail;

  size = ftell (file);
  if (size < 0) goto bail;

  ret = fseek (file, 0, SEEK_SET);
  if (ret < 0) goto bail;

  contents = malloc (size + 1);
  if (contents == NULL) goto bail;

  ret = fread (contents, size, 1, file);
  if (ret < 0) goto bail;

  contents[size] = 0;

  return contents;
bail:
  /* something failed */
  if (file) fclose (file);
  if (contents) free (contents);

  return NULL;
}

const char *varnames[] = {
  "d1", "d2", "d3", "d4",
  "s1", "s2", "s3", "s4",
  "s5", "s6", "s7", "s8",
  "a1", "a2", "a3", "d4",
  "c1", "c2", "c3", "c4",
  "c5", "c6", "c7", "c8",
  "p1", "p2", "p3", "p4",
  "p5", "p6", "p7", "p8",
  "t1", "t2", "t3", "t4",
  "t5", "t6", "t7", "t8",
  "t9", "t10", "t11", "t12",
  "t13", "t14", "t15", "t16"
};

const char *enumnames[] = {
  "ORC_VAR_D1", "ORC_VAR_D2", "ORC_VAR_D3", "ORC_VAR_D4",
  "ORC_VAR_S1", "ORC_VAR_S2", "ORC_VAR_S3", "ORC_VAR_S4",
  "ORC_VAR_S5", "ORC_VAR_S6", "ORC_VAR_S7", "ORC_VAR_S8",
  "ORC_VAR_A1", "ORC_VAR_A2", "ORC_VAR_A3", "ORC_VAR_A4",
  "ORC_VAR_C1", "ORC_VAR_C2", "ORC_VAR_C3", "ORC_VAR_C4",
  "ORC_VAR_C5", "ORC_VAR_C6", "ORC_VAR_C7", "ORC_VAR_C8",
  "ORC_VAR_P1", "ORC_VAR_P2", "ORC_VAR_P3", "ORC_VAR_P4",
  "ORC_VAR_P5", "ORC_VAR_P6", "ORC_VAR_P7", "ORC_VAR_P8",
  "ORC_VAR_T1", "ORC_VAR_T2", "ORC_VAR_T3", "ORC_VAR_T4",
  "ORC_VAR_T5", "ORC_VAR_T6", "ORC_VAR_T7", "ORC_VAR_T8",
  "ORC_VAR_T9", "ORC_VAR_T10", "ORC_VAR_T11", "ORC_VAR_T12",
  "ORC_VAR_T13", "ORC_VAR_T14", "ORC_VAR_T15", "ORC_VAR_T16"
};

void
output_prototype (OrcProgram *p, FILE *output)
{
  OrcVariable *var;
  int i;
  int need_comma;

  fprintf(output, "%s (", p->name);
  need_comma = FALSE;
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_D1 + i];
    if (var->size) {
      if (need_comma) fprintf(output, ", ");
      if (var->type_name) {
        fprintf(output, "%s * %s", var->type_name,
            varnames[ORC_VAR_D1 + i]);
      } else {
        fprintf(output, "uint%d_t * %s", var->size*8,
            varnames[ORC_VAR_D1 + i]);
      }
      need_comma = TRUE;
    }
  }
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_A1 + i];
    if (var->size) {
      if (need_comma) fprintf(output, ", ");
      if (var->type_name) {
        fprintf(output, "%s * %s", var->type_name,
            varnames[ORC_VAR_A1 + i]);
      } else {
        fprintf(output, "uint%d_t * %s", var->size*8,
            varnames[ORC_VAR_A1 + i]);
      }
      need_comma = TRUE;
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_S1 + i];
    if (var->size) {
      if (need_comma) fprintf(output, ", ");
      if (var->type_name) {
        fprintf(output, "%s * %s", var->type_name,
            varnames[ORC_VAR_S1 + i]);
      } else {
        fprintf(output, "uint%d_t * %s", var->size*8,
            varnames[ORC_VAR_S1 + i]);
      }
      need_comma = TRUE;
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_P1 + i];
    if (var->size) {
      if (need_comma) fprintf(output, ", ");
      fprintf(output, "int %s", varnames[ORC_VAR_P1 + i]);
      need_comma = TRUE;
    }
  }
  if (p->constant_n == 0) {
    if (need_comma) fprintf(output, ", ");
    fprintf(output, "int n");
    need_comma = TRUE;
  }
  if (p->is_2d && p->constant_m == 0) {
    if (need_comma) fprintf(output, ", ");
    fprintf(output, "int m");
  }
  fprintf(output, ")");
}

void
output_code_header (OrcProgram *p, FILE *output)
{
  fprintf(output, "void ");
  output_prototype (p, output);
  fprintf(output, ";\n");
}

void
output_code_backup (OrcProgram *p, FILE *output)
{

  fprintf(output, "static void\n");
  fprintf(output, "_backup_%s (OrcExecutor *ex)\n", p->name);
  fprintf(output, "{\n");
  {
    OrcCompileResult result;

    result = orc_program_compile_full (p, orc_target_get_by_name("c"),
        ORC_TARGET_C_BARE);
    if (ORC_COMPILE_RESULT_IS_SUCCESSFUL(result)) {
      fprintf(output, "%s\n", orc_program_get_asm_code (p));
    }
  }
  fprintf(output, "}\n");
  fprintf(output, "\n");

}

void
output_code (OrcProgram *p, FILE *output)
{
  OrcVariable *var;
  int i;

  fprintf(output, "\n");
  fprintf(output, "/* %s */\n", p->name);
  output_code_backup (p, output);
  fprintf(output, "void\n");
  output_prototype (p, output);
  fprintf(output, "\n");
  fprintf(output, "{\n");
  fprintf(output, "  static int p_inited = 0;\n");
  fprintf(output, "  static OrcProgram *p = NULL;\n");
  fprintf(output, "  OrcExecutor _ex, *ex = &_ex;\n");
  fprintf(output, "\n");
  fprintf(output, "  if (!p_inited) {\n");
  fprintf(output, "    orc_once_mutex_lock ();\n");
  fprintf(output, "    if (!p_inited) {\n");
  fprintf(output, "      OrcCompileResult result;\n");
  fprintf(output, "\n");
  fprintf(output, "      p = orc_program_new ();\n");
  if (p->constant_n != 0) {
    fprintf(output, "      orc_program_set_constant_n (p, %d);\n",
        p->constant_n);
  }
  if (p->is_2d) {
    fprintf(output, "      orc_program_set_2d (p);\n");
    if (p->constant_m != 0) {
      fprintf(output, "      orc_program_set_constant_m (p, %d);\n",
          p->constant_m);
    }
  }
  fprintf(output, "      orc_program_set_name (p, \"%s\");\n", p->name);
  fprintf(output, "      orc_program_set_backup_function (p, _backup_%s);\n",
      p->name);
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_D1 + i];
    if (var->size) {
      fprintf(output, "      orc_program_add_destination (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_D1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_S1 + i];
    if (var->size) {
      fprintf(output, "      orc_program_add_source (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_S1 + i]);
    }
  }
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_A1 + i];
    if (var->size) {
      fprintf(output, "      orc_program_add_accumulator (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_A1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_C1 + i];
    if (var->size) {
      if (var->value != 0x80000000) {
        fprintf(output, "      orc_program_add_constant (p, %d, %u, \"%s\");\n",
            var->size, var->value, varnames[ORC_VAR_C1 + i]);
      } else {
        fprintf(output, "      orc_program_add_constant (p, %d, 0x%08x, \"%s\");\n",
            var->size, var->value, varnames[ORC_VAR_C1 + i]);
      }
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_P1 + i];
    if (var->size) {
      fprintf(output, "      orc_program_add_parameter (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_P1 + i]);
    }
  }
  for(i=0;i<16;i++){
    var = &p->vars[ORC_VAR_T1 + i];
    if (var->size) {
      fprintf(output, "      orc_program_add_temporary (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_T1 + i]);
    }
  }
  fprintf(output, "\n");

  for(i=0;i<p->n_insns;i++){
    OrcInstruction *insn = p->insns + i;
    if (p->vars[insn->src_args[1]].size != 0) {
      fprintf(output, "      orc_program_append (p, \"%s\", %s, %s, %s);\n",
          insn->opcode->name, enumnames[insn->dest_args[0]],
          enumnames[insn->src_args[0]], enumnames[insn->src_args[1]]);
    } else {
      fprintf(output, "      orc_program_append_ds (p, \"%s\", %s, %s);\n",
          insn->opcode->name, enumnames[insn->dest_args[0]],
          enumnames[insn->src_args[0]]);
    }
  }

  fprintf(output, "\n");
  fprintf(output, "      result = orc_program_compile (p);\n");
#if 0
  /* don't care.  we have a backup function */
  fprintf(output, "      if (!ORC_COMPILE_RESULT_IS_SUCCESSFUL (result)) {\n");
  fprintf(output, "        abort ();\n");
  fprintf(output, "      }\n");
#endif
  fprintf(output, "    }\n");
  fprintf(output, "    p_inited = TRUE;\n");
  fprintf(output, "    orc_once_mutex_unlock ();\n");
  fprintf(output, "  }\n");
  fprintf(output, "\n");
  //fprintf(output, "  orc_executor_set_program (ex, p);\n");
  fprintf(output, "  ex->program = p;\n");
  //fprintf(output, "  orc_executor_set_n (ex, n);\n");
  if (p->constant_n) {
    fprintf(output, "  ex->n = %d;\n", p->constant_n);
  } else {
    fprintf(output, "  ex->n = n;\n");
  }
  if (p->is_2d) {
    if (p->constant_m) {
      fprintf(output, "  ORC_EXECUTOR_M(ex) = %d;\n", p->constant_m);
    } else {
      fprintf(output, "  ORC_EXECUTOR_M(ex) = m;\n");
    }
  }
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_D1 + i];
    if (var->size) {
      //fprintf(output, "  orc_executor_set_array (ex, %s, %s);\n",
      //    enumnames[ORC_VAR_D1 + i], varnames[ORC_VAR_D1 + i]);
      fprintf(output, "  ex->arrays[%s] = %s;\n",
          enumnames[ORC_VAR_D1 + i], varnames[ORC_VAR_D1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_S1 + i];
    if (var->size) {
      //fprintf(output, "  orc_executor_set_array (ex, %s, %s);\n",
      //    enumnames[ORC_VAR_S1 + i], varnames[ORC_VAR_S1 + i]);
      fprintf(output, "  ex->arrays[%s] = %s;\n",
          enumnames[ORC_VAR_S1 + i], varnames[ORC_VAR_S1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_P1 + i];
    if (var->size) {
      //fprintf(output, "  orc_executor_set_param (ex, %s, %s);\n",
      //    enumnames[ORC_VAR_P1 + i], varnames[ORC_VAR_P1 + i]);
      fprintf(output, "  ex->params[%s] = %s;\n",
          enumnames[ORC_VAR_P1 + i], varnames[ORC_VAR_P1 + i]);
    }
  }
  fprintf(output, "\n");
  fprintf(output, "  orc_executor_run (ex);\n");
  //fprintf(output, "  ((void (*)(OrcExecutor *))ex->program->code_exec)(ex);\n");
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_A1 + i];
    if (var->size) {
      fprintf(output, "  *%s = orc_executor_get_accumulator (ex, %s);\n",
          varnames[ORC_VAR_A1 + i], enumnames[ORC_VAR_A1 + i]);
    }
  }
  fprintf(output, "}\n");

}

void
output_code_test (OrcProgram *p, FILE *output)
{
  OrcVariable *var;
  int i;

  fprintf(output, "  /* %s */\n", p->name);
  fprintf(output, "  {\n");
  fprintf(output, "    OrcProgram *p = NULL;\n");
  fprintf(output, "    int ret;\n");
  fprintf(output, "\n");
  fprintf(output, "    printf (\"%s:\\n\");\n", p->name);
  fprintf(output, "    p = orc_program_new ();\n");
  if (p->constant_n != 0) {
    fprintf(output, "      orc_program_set_constant_n (p, %d);\n",
        p->constant_n);
  }
  if (p->is_2d) {
    fprintf(output, "      orc_program_set_2d (p);\n");
    if (p->constant_m != 0) {
      fprintf(output, "      orc_program_set_constant_m (p, %d);\n",
          p->constant_m);
    }
  }
  fprintf(output, "    orc_program_set_name (p, \"%s\");\n", p->name);
  fprintf(output, "    orc_program_set_backup_function (p, _backup_%s);\n",
      p->name);
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_D1 + i];
    if (var->size) {
      fprintf(output, "    orc_program_add_destination (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_D1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_S1 + i];
    if (var->size) {
      fprintf(output, "    orc_program_add_source (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_S1 + i]);
    }
  }
  for(i=0;i<4;i++){
    var = &p->vars[ORC_VAR_A1 + i];
    if (var->size) {
      fprintf(output, "    orc_program_add_accumulator (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_A1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_C1 + i];
    if (var->size) {
      fprintf(output, "    orc_program_add_constant (p, %d, %d, \"%s\");\n",
          var->size, var->value, varnames[ORC_VAR_C1 + i]);
    }
  }
  for(i=0;i<8;i++){
    var = &p->vars[ORC_VAR_P1 + i];
    if (var->size) {
      fprintf(output, "    orc_program_add_parameter (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_P1 + i]);
    }
  }
  for(i=0;i<16;i++){
    var = &p->vars[ORC_VAR_T1 + i];
    if (var->size) {
      fprintf(output, "    orc_program_add_temporary (p, %d, \"%s\");\n",
          var->size, varnames[ORC_VAR_T1 + i]);
    }
  }
  fprintf(output, "\n");

  for(i=0;i<p->n_insns;i++){
    OrcInstruction *insn = p->insns + i;
    if (insn->src_args[1] != -1) {
      fprintf(output, "    orc_program_append (p, \"%s\", %s, %s, %s);\n",
          insn->opcode->name, enumnames[insn->dest_args[0]],
          enumnames[insn->src_args[0]], enumnames[insn->src_args[1]]);
    } else {
      fprintf(output, "    orc_program_append_ds (p, \"%s\", %s, %s);\n",
          insn->opcode->name, enumnames[insn->dest_args[0]],
          enumnames[insn->src_args[0]]);
    }
  }

  fprintf(output, "\n");
  fprintf(output, "    ret = orc_test_compare_output_backup (p);\n");
  fprintf(output, "    if (!ret) {\n");
  fprintf(output, "      error = TRUE;\n");
  fprintf(output, "    }\n");
  fprintf(output, "\n");
  fprintf(output, "    ret = orc_test_compare_output (p);\n");
  fprintf(output, "    if (!ret) {\n");
  fprintf(output, "      error = TRUE;\n");
  fprintf(output, "    }\n");
  fprintf(output, "\n");
  fprintf(output, "    orc_program_free (p);\n");
  fprintf(output, "  }\n");
  fprintf(output, "\n");

}

