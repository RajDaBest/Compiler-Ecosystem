import * as vscode from 'vscode';

const INSTRUCTION_SET = [
    'nop', 'spush', 'fpush', 'upush', 'rdup', 'adup',
    'splus', 'uplus', 'fplus', 'sminus', 'uminus', 'fminus',
    'smult', 'umult', 'fmult', 'sdiv', 'udiv', 'fdiv',
    'jmp', 'halt', 'ujmp_if', 'fjmp_if', 'eq', 'lsr', 'asr',
    'sl', 'and', 'or', 'not', 'empty', 'pop_at', 'pop',
    'rswap', 'aswap', 'ret', 'call', 'native',
    'zeload8', 'zeload16', 'zeload32', 'load64',
    'seload8', 'seload16', 'seload32',
    'store8', 'store16', 'store32', 'store64',
    'equ', 'eqs', 'eqf', 'geu', 'ges', 'gef',
    'leu', 'les', 'lef', 'gu', 'gs', 'gf',
    'lu', 'ls', 'lf',
    'utf', 'stf', 'ftu', 'fts', 'stu', 'uts' // Added new instructions here
];

const DIRECTIVES = [
    '%include', '%define',
    '.byte', '.string', '.double', '.word', '.doubleword', '.quadword',
    '.text', '.data'
];

function getInstructionDocumentation(instruction: string): string {
    const docs: { [key: string]: string } = {
        'nop': 'No operation',
        'spush': 'Push signed integer onto stack - Format: spush <value>',
        'fpush': 'Push floating-point number onto stack - Format: fpush <value>',
        'upush': 'Push unsigned integer onto stack - Format: upush <value>',
        'rdup': 'Duplicate stack element relative to top - Format: rdup <offset>',
        'adup': 'Duplicate stack element at absolute position - Format: adup <position>',
        'splus': 'Add signed integers: dest = dest + src, pop src',
        'uplus': 'Add unsigned integers: dest = dest + src, pop src',
        'fplus': 'Add floating-point numbers: dest = dest + src, pop src',
        'sminus': 'Subtract signed integers: dest = dest - src, pop src',
        'uminus': 'Subtract unsigned integers: dest = dest - src, pop src',
        'fminus': 'Subtract floating-point numbers: dest = dest - src, pop src',
        'smult': 'Multiply signed integers: dest = dest * src, pop src',
        'umult': 'Multiply unsigned integers: dest = dest * src, pop src',
        'fmult': 'Multiply floating-point numbers: dest = dest * src, pop src',
        'sdiv': 'Divide signed integers: dest = dest / src, pop src',
        'udiv': 'Divide unsigned integers: dest = dest / src, pop src',
        'fdiv': 'Divide floating-point numbers: dest = dest / src, pop src',
        'jmp': 'Unconditional jump to address in text section - Format: jmp <address>',
        'ujmp_if': 'Jump if top of stack is non-zero (unsigned) - Format: ujmp_if <address>',
        'fjmp_if': 'Jump if top of stack is not zero (float, epsilon = 1e-9) - Format: fjmp_if <address>',
        'eq': 'Compare equality: dest = (dest == src), pop src',
        'lsr': 'Logical shift right - Format: lsr <count>',
        'asr': 'Arithmetic shift right - Format: asr <count>',
        'sl': 'Shift left - Format: sl <count>',
        'and': 'Bitwise AND: dest = dest & src, pop src',
        'or': 'Bitwise OR: dest = dest | src, pop src',
        'not': 'Bitwise NOT: dest = ~dest',
        'empty': 'Check if stack is empty, push result (0 or 1)',
        'pop_at': 'Pop element at specific stack position - Format: pop_at <position>',
        'pop': 'Pop top element from stack',
        'rswap': 'Swap stack top with relative position - Format: rswap <offset>',
        'aswap': 'Swap stack top with absolute position - Format: aswap <position>',
        'ret': 'Return from function call',
        'call': 'Call function at address - Format: call <address>',
        'native': 'Call native function - Format: native <function_id>',
        'halt': 'Stop program execution',
        'zeload8': 'Load 8 bits from memory and zero extend',
        'zeload16': 'Load 16 bits from memory and zero extend',
        'zeload32': 'Load 32 bits from memory and zero extend',
        'load64': 'Load 64 bits from memory',
        'seload8': 'Load 8 bits from memory and sign extend',
        'seload16': 'Load 16 bits from memory and sign extend',
        'seload32': 'Load 32 bits from memory and sign extend',
        'store8': 'Store 8 bits to memory',
        'store16': 'Store 16 bits to memory',
        'store32': 'Store 32 bits to memory',
        'store64': 'Store 64 bits to memory',

        // New comparison instructions
        'equ': 'Compare equality: dest = (dest == src), pop src',
        'eqs': 'Compare equality for signed integers: dest = (dest == src), pop src',
        'eqf': 'Compare equality for floating-point numbers: dest = (dest == src), pop src',
        'geu': 'Compare unsigned integers: dest = (dest >= src), pop src',
        'ges': 'Compare signed integers: dest = (dest >= src), pop src',
        'gef': 'Compare floating-point numbers: dest = (dest >= src), pop src',
        'leu': 'Compare unsigned integers: dest = (dest <= src), pop src',
        'les': 'Compare signed integers: dest = (dest <= src), pop src',
        'lef': 'Compare floating-point numbers: dest = (dest <= src), pop src',
        'gu': 'Compare greater than for unsigned integers: dest = (dest > src), pop src',
        'gs': 'Compare greater than for signed integers: dest = (dest > src), pop src',
        'gf': 'Compare greater than for floating-point numbers: dest = (dest > src), pop src',
        'lu': 'Compare less than for unsigned integers: dest = (dest < src), pop src',
        'ls': 'Compare less than for signed integers: dest = (dest < src), pop src',
        'lf': 'Compare less than for floating-point numbers: dest = (dest < src), pop src',

        'utf': 'Convert top of stack from unsigned to signed',
        'stf': 'Convert top of stack from signed to floating-point',
        'ftu': 'Convert top of stack from floating-point to unsigned',
        'fts': 'Convert top of stack from floating-point to signed',
        'stu': 'Convert top of stack from signed to unsigned',
        'uts': 'Convert top of stack from unsigned to signed'
    };

    return docs[instruction] || 'No documentation available';
}
