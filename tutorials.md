---
layout: default
title: tutorials
---
### What it does?

a subset of features:

<ul>
  {% for page in site.pages %}
  {% if page.tags contains 'example' %}
    <li>
      <a href="{{site.baseurl}}{{ page.url }}">{{ page.title }}</a>
    </li>
  {% endif %}
  {% endfor %}
</ul>
