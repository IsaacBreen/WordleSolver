const { Worker, isMainThread, parentPort } = require('worker_threads');
if (isMainThread) {
    // This code is executed in the main thread and not in the worker.

    // Create the worker.
    const worker = new Worker(__filename);
    // Listen for messages from the worker and print them.
    worker.on('message', (msg) => { console.log(msg); });
} else {
    // This code is executed in the worker and not in the main thread.

    // Send a message to the main thread.
    parentPort.postMessage('Hello world!');
}
