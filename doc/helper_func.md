# Helper Functions and Classes

Various helper functions and classes are provided.

## <a name='Contents'></a>Contents

<!-- vscode-markdown-toc -->
* [Contents](#Contents)
* [LimitTimer](#LimitTimer)
* [License](#License)

<!-- vscode-markdown-toc-config
	numbering=false
	autoSave=true
	/vscode-markdown-toc-config -->
<!-- /vscode-markdown-toc -->



## <a name='LimitTimer'></a>LimitTimer

The LimitTimer can be used to avoid blocking delay and while.

    LimitTimer timer(50); // Timer every 50 ms
    LimitTimer timer(2000, 7); // Timer every 2 s but stop after 7 times

The first call of justFinished() will always be true and start the timer from here.

    if (timer.justFinished()) { // First call and interval has passed } ;

    if (timer.running()) { // It was started and limit is not reached }

    timer.reset(); // Restart

    if (timer.plusOne()) { // Single additional interval after the limit was reached }



## <a name='License'></a>License

Licensed under the Apache License. See [LICENSE](/LICENSE) for more information.

Copyright Production 3000