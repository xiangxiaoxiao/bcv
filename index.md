---
layout: default
title: bcv
---
### What this is

bcv is a small C++ library of algorithms that i (personally) find interesting. 

* [Changelog](changelog.html)
* [Installation tips](installation.html)

### What it does?

<ul>
  {% for page in site.pages %}
  {% if page.tags contains 'example' %}
    <li>
      <a href="{{site.baseurl}}{{ page.url }}">{{ page.title }}</a>
    </li>
  {% endif %}
  {% endfor %}
</ul>

### Download

* [ZIP](https://github.com/vasiliykarasev/bcv/zipball/master)
* [TAR](https://github.com/vasiliykarasev/bcv/tarball/master)
* [View on *GitHub*](https://github.com/vasiliykarasev/bcv)


### Contact

[email me](mailto:karasev00@gmail.com)
