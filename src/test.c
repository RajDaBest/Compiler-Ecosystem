if (is_nan(program[i].operand))
            {
                printf("%s %ld\n", inst_type_as_asm_str(type), return_value_signed(program[i].operand));
            }
            else
            {
                printf("%s %lf\n", inst_type_as_asm_str(type), program[i].operand);
            }
            break;