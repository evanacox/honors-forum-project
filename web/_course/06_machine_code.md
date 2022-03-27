---
title: "Machine Code: The Final Step"
permalink: /articles/crashcourse/machinecode/
excerpt: "The final step between our code and a functional `.exe` file"
usemathjax: true
classes: wide
---

If you go back all the way page #2 in this series, you'll
remember that the overarching goal of this process is to
get something useful from our code: an `.exe`.

Unfortunately, making an `.exe` is not quite as simple
as just translating all of our code into machine code
and dumping it into a file, it's *slightly* more complicated
than that.

> *Note: For the sake of understanding, I am brushing over*
> *an absolutely **massive** amount complexity. The modern `.exe`*
> *format is the result of nearly 40+ years of development and*
> *organic growth, and trying to put it through an analogy does*
> *not truly give it justice.* 
> 
> *See [PE](https://en.wikipedia.org/wiki/Portable_Executable),*
> *[ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format),*
> *[Linking](https://en.wikipedia.org/wiki/Linker_(computing))*
> *and [Loading](https://en.wikipedia.org/wiki/Loader_(computing))*
> *for a more accurate understanding of what actually goes on*
> *if you want to know how the linking actually works on different*
> *operating systems.*

## Understanding `.exe`s

An `.exe` file is effectively a very well-organized filing cabinet,
with each folder in that cabinet filled to the brim with code.

All it does at its core is group together related bits of code, and 
keep them in the right place so that our computer can actually read
that code and execute it. 

When your computer goes to run an `.exe`, it doesn't know a thing
about what any of those folders are, what's in them, or what any of
it means. But, it *can* just ask the filing cabinet to tell it
where in the filing cabinet it should start executing machine code!

That's exactly what an `.exe` does: it puts information in a very
specific place in this filing cabinet, and your computer can go
and look at that specific place to go see where it needs to start
executing instructions. 

![filing cabinet](/assets/images/crash-course/machine-code/cabinet.png)

## Populating Folders

Before we can give a filing cabinet to the computer though, we
have to actually *create* that filing cabinet. 

In doing so, we need to break our program into chunks, and decide
how we want to organize it. Once we've done that, we can then
translate each chunk of our program into machine code, and populate
that folder of the cabinet.

## Putting Them Into the Cabinet

Once we've got a bunch of folders, we need to do something with them. In order to do that, we need to *link* them together.

*Linking* is the computing term for "stitching together chunks
of machine code," in this case we're doing that and putting them
together in a way that we can then put in our filing cabinet.

## Wrapping Up

And that's it! Once we've linked our chunks of code together and 
made our metaphorical "filing cabinet," we're done! We can do
whatever we like with the filing cabinet (`.exe` file we've created),
whether it be uploading it, selling it, etc. 

We have finished our job, we have successfully translated some code
into machine code, and packaged it into something useful that we can
now use to accomplish our task.