---
layout: default
title: Input/output in BCV
tags: example
---
## {{page.title}} (test_io)

This example demonstrates how to read/write images and videos using BCV.

### I/O with images

Reading images is easy:

{% highlight cpp %}
int rows, cols, chan;
vector<uchar> img = bcv::bcv_imread<uchar>("my-image.jpg", &rows, &cols, &chan);
{% endhighlight %}

If you want a grayscale image (independent of whether the actual image on disk is grayscale or not), 
you can omit the `chan` argument:

{% highlight cpp %}
vector<uchar> img = bcv::bcv_imread<uchar>("my-image.jpg", &rows, &cols);
{% endhighlight %}

BCV only supports `jpg` and `png`, and requires that the extension matches the codec (i.e. it will not allow you to load "my-image.sup" even if the image is actually a jpeg). An image is just a STL vector. Data order is similar to opencv -- channels are fastest changing, followed by columns, and lastly rows.

Writing the images is quite similar:

{% highlight cpp %}
bcv::bcv_imwrite<uchar>("my-future-image.jpg", img, rows, cols, chan);
{% endhighlight %}

If the image is grayscale, you can again omit the `chan` argument.

### I/O with videos

Videos are somewhat more complex, as will be shown below.
You can open videos as follows:

{% highlight cpp %}
video_reader f;
f.open("my-video.mp4");
if (!f.is_opened()) { /* handle error somehow */ }
cols = f.get_width();
rows = f.get_height();
vector<uchar> img = f.get_frame<uchar>();
...
f.close();
{% endhighlight %}

You could combine the call to `open` with the constructor and the call to `close`
with the destructor, and just do:

{% highlight cpp %}
video_reader f("my-video.mp4");
if (!f.is_opened()) { /* handle error somehow */ }
cols = f.get_width();
rows = f.get_height()
vector<uchar> img = f.get_frame<uchar>();
{% endhighlight %}

In both cases, you *must* check that the file has been successfully open. If it
was opened, you can get frames in linear order using `get_frame`. Once there 
are no frames left in the video, `get_frame` returns an empty vector. Thus, you
can check for end-of-video using

{% highlight cpp %}
vector<uchar> img = f.get_frame<uchar>();
if (img.empty()) { /* reached end of video */ }
{% endhighlight %}

Note that frames in the video are accessed in linear order, and cannot be "rewinded".

      
<br>    
    
The process of writing videos is similar in some aspects.

{% highlight cpp %}
video_writer f("hello-my-future-video.mp4", width, height);
if (!f.is_opened()) { /* handle basic sanity errors */ }
f.set_fps(10);
f.set_bitrate(10000000);
if (!f.prepare()) { /* handle errors */ }
while (1) { f.add_frame( img ); }
f.close()
{% endhighlight %}

Opening and closing an *output* video is similar to opening/closing
an *input* video. the `close` call can be combined with the destructor and the `open`
call can be combined with the constructor.

As above, you need to check whether the file can be successfully opened, using `is_opened()`.

Additionally, the call to `prepare()` ensures that frames can be written to the video. Frames
are added to the video in linear order using `add_frame`.

