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
    'lu', 'ls', 'lf'
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
    };

    return docs[instruction] || 'No documentation available';
}

function getDirectiveDocumentation(directive: string): string {
    const docs: { [key: string]: string } = {
        '%include': 'Include another VASM file - Format: %include "filename"',
        '%define': 'Define a constant - Format: %define NAME VALUE',
        '.byte': 'Define a byte (8 bits) - Format: .byte <value>',
        '.string': 'Define a null-terminated string - Format: .string "text"',
        '.double': 'Define a double-precision floating-point number (64 bits) - Format: .double <value>',
        '.word': 'Define a word (16 bits) - Format: .word <value>',
        '.doubleword': 'Define a doubleword (32 bits) - Format: .doubleword <value>',
        '.quadword': 'Define a quadword (64 bits) - Format: .quadword <value>',
        '.text': 'Begin text section (code)',
        '.data': 'Begin data section'
    };

    return docs[directive] || 'No documentation available';
}

export function activate(context: vscode.ExtensionContext) {
    // Register completions provider
    const completionProvider = vscode.languages.registerCompletionItemProvider(
        'vasm',
        {
            provideCompletionItems(document: vscode.TextDocument, position: vscode.Position) {
                const completions: vscode.CompletionItem[] = [];

                // Add instructions
                INSTRUCTION_SET.forEach(instruction => {
                    const item = new vscode.CompletionItem(instruction, vscode.CompletionItemKind.Keyword);
                    item.documentation = new vscode.MarkdownString(getInstructionDocumentation(instruction));
                    completions.push(item);
                });

                // Add directives
                DIRECTIVES.forEach(directive => {
                    const item = new vscode.CompletionItem(directive, vscode.CompletionItemKind.Keyword);
                    item.documentation = new vscode.MarkdownString(getDirectiveDocumentation(directive));
                    completions.push(item);
                });

                return completions;
            }
        }
    );

    // Register hover provider
    const hoverProvider = vscode.languages.registerHoverProvider(
        'vasm',
        {
            provideHover(document: vscode.TextDocument, position: vscode.Position) {
                const range = document.getWordRangeAtPosition(position);
                const word = document.getText(range);

                if (INSTRUCTION_SET.includes(word)) {
                    return new vscode.Hover(getInstructionDocumentation(word));
                }

                if (DIRECTIVES.includes(word)) {
                    return new vscode.Hover(getDirectiveDocumentation(word));
                }

                return null;
            }
        }
    );

    // Register diagnostic provider
    let diagnosticCollection = vscode.languages.createDiagnosticCollection('vasm');

    let activeEditor = vscode.window.activeTextEditor;

    if (activeEditor) {
        updateDiagnostics(activeEditor.document, diagnosticCollection);
    }

    context.subscriptions.push(
        vscode.workspace.onDidChangeTextDocument(editor => {
            if (activeEditor && editor.document === activeEditor.document) {
                updateDiagnostics(editor.document, diagnosticCollection);
            }
        })
    );

    context.subscriptions.push(
        vscode.window.onDidChangeActiveTextEditor(editor => {
            activeEditor = editor;
            if (editor) {
                updateDiagnostics(editor.document, diagnosticCollection);
            }
        })
    );

    context.subscriptions.push(completionProvider, hoverProvider);
}

function updateDiagnostics(document: vscode.TextDocument, collection: vscode.DiagnosticCollection): void {
    if (document.languageId !== 'vasm') {
        return;
    }

    const diagnostics: vscode.Diagnostic[] = [];

    for (let i = 0; i < document.lineCount; i++) {
        const line = document.lineAt(i);
        const text = line.text.trim();

        if (text === '') continue;

        // Check for invalid instructions
        const words = text.split(/\s+/);
        const instruction = words[0];

        if (!instruction.startsWith(';') && !instruction.endsWith(':') &&
            !INSTRUCTION_SET.includes(instruction) && !DIRECTIVES.includes(instruction)) {
            const diagnostic = new vscode.Diagnostic(
                line.range,
                `Unknown instruction or directive: ${instruction}`,
                vscode.DiagnosticSeverity.Error
            );
            diagnostics.push(diagnostic);
        }
    }

    collection.set(document.uri, diagnostics);
}

export function deactivate() { }