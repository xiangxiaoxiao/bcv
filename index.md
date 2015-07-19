---
layout: default
title: bcv
---
### What this is

bcv is a small library of algorithms that i (personally) find interesting. 

### What it does?

<ul>
  {% for page in site.pages %}
  {% if page.tags contains 'example' %}
    <li>
      <a href="{{site.url}}{{ page.siteurl }}">{{ page.title }}</a>
    </li>
  {% endif %}
  {% endfor %}
</ul>

### Download

* [ZIP](https://github.com/vasiliykarasev/bcv/zipball/master)
* [TAR](https://github.com/vasiliykarasev/bcv/tarball/master)
* [View on *GitHub*](https://github.com/vasiliykarasev/bcv)
* [Installation tips](installation.html)
* [Changelog](changelog.html)

### Contact

[email me](mailto:karasev00@gmail.com)
