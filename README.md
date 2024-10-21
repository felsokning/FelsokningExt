# FelsokningExt
Just another Windbg Extension

## deep
`deep` allows you to traverse all threads in the dump (or live process - not yet tested) to find threads that are above `x` frames.

### Loading the Extension
Download the target flavor from [Releases](https://github.com/felsokning/FelsokningExt/releases) and extract the contents. You should place the dll in a folder you'll recall, later, as you'll need to type the path into Windbg to load the extension.

For example, on my local machine, the load command would be: `.load C:\ext\FelsokningExt.dll`

### Usage
`!FelsokningExt.deep <number>` or `!deep <number>` will resolve in Windbg and output the results.

### Feature
For each thread found above the target frame size, the thread id is output as a hyperlink (recognisable only to Windbg), which you can click on to change to that thread's context. Below is example of this from a dump I took of the Spotify process.

![Image showing thread hypelink](.\images\thread_hyperlink.PNG)

