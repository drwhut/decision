# Contributing to Decision

This software started life as an experiment, to see if I could actually create
a programming language. Now it is in a released state, I have now only just
realised I may have bitten more than I can chew... Which is why if you like the
look of this project and want to help contribute, this document states the many
ways in which you can!

## 1. Try it out!

If you are just using the language, **it is still contributing!** Feel free to
either download a binary if one is available, or compile it from source (which
is easier than it sounds, have a look at [README.md](README.md)), and go wild!

### I've found a bug and I want it DESTROYED.

If you feel like helping squishing something, let me know by creating an
**issue** with the **Bug** label, if hasn't already been spotted. In the issue,
try and put the following information in, which helps me help you:

* What was the error displayed?
* What were the contents of the source file(s) you tried to run?
* What was the command you used to run the above source file(s)?
* What system are you running Decision on? (e.g. 64-bit Windows)

### I can't do a thing and I want to do the thing that I can't do.

Does the compiler not meet your requirements? Let me know by creating an
**issue** with the **Feature** label, if someone else hasn't already thought
of it.

Keep in mind that not every feature everyone suggests will be implemented - if
a developer decides that the feature should only be partially implemented, or
not implemented at all, they will tell you in the comments of the issue, and
may also suggest a possible workaround if one exists.

### I can't understand a single word you've written.

Struggling to understand something in any part of the documentation? Let me
know by creating an **issue** with the **Documentation** label, if someone else
hasn't had the same difficulty.

As the developer of this project, I basically know the language inside and out.
But I know that no-one else does, which is why I created the documentation.
I also want the documentation to be as easy to understand as possible. So if
it isn't, *please* let me know so I can help you as well as others to get their
foot in the door in terms of using the language.

## 2. Have a look at the code!

If you are even slightly curious as to how the language works, go ahead!
Try starting with [dmain.c](src/dmain.c), which contains the `main` execution
point of the compiler, and go from there!

Remember that there is a **developer manual** designed for anyone who is
looking at the source code, which explains each stage of compilation in detail.
It also has a reference to every structure, function, etc. in the header files
with comments for almost everything.

### I see you're doing something this way, but I've thought of a better way.

If you know how to optimise a piece of code, let me know by creating an
**issue** with the **Optimisation** label, if someone hasn't already spotted
the same thing.

## 3. Change something!

If you see an **issue**, and feel confident in fixing the said issue, then
here's what you can do:

1. Check to see if a branch has been made for the issue in question, and see
how much progress has been made in solving the issue. If a branch has not yet
been made, keep bugging me.

2. Fork the repository and make the changes you want to make on your repo!

3. Create a pull request with clear detail as to what your changes achieve.
Ensure that before you create the pull request, your fork is up-to-date with
the upstream branch!

4. If the pull request has been accepted, *great!* Otherwise, there was
probably a reason for it, which should be mentioned in the comments.

**NOTE:** In terms of keeping the code consistent, there is a `.clang-format`
file which is used to format all source files consistently.