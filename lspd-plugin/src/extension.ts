import * as vscode from 'vscode';


export function activate(context: vscode.ExtensionContext) {
    console.log("lspd plugin activated")
    
    const disposable = vscode.commands.registerCommand('lspd.helloWorld', () => {
        vscode.window.showInformationMessage('Hello World from lspd!');
    });
    context.subscriptions.push(disposable);
}

export function deactivate() {}


