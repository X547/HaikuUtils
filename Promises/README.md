Promises

Simple promise implementation and demonstration of using promises to request user input with dialog window.

Promises allow to run some task asynchronously and register callbacks that will be called when task is completed or failed. No threads are used by promises itself, it only register and call completion callbacks.

Example use promise to ask user to enter login and password in a window. Then user enter form, promise will be resolved and all resolve callbacks will be called, receiving entered credentials. user also can cancel dialog that will cause reject callbacks to be called. Promise client can also reject promise it it don't need for data anymore, that will cause form dialog to be closed.

![screenshot](https://raw.githubusercontent.com/X547/HaikuUtils/master/Promises/screenshot.png)
