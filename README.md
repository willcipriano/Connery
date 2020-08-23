<p align="center">
  <img width="368" height="414.5" src="http://connerylang.org/img/ConneryLogo.jpg">
</p>
<p align="center">Connery is a experimental lisp-like interpreted programming language that is somewhat themed after veteran actor Sean Connery.</p>
<p align="center"><a href="http://connerylang.org">ConneryLang.org</a></p>

## Building and running Connery
The best way to get up and running is clone this repo and allow the Dockerfile to pull all the dependencies for you. This method will drop you right into the Connery REPL.
```
1. git clone https://github.com/willcipriano/Connery.git
2. cd Connery
3. docker build --tag connery .
4. docker run -it connery
```

![GitHub tag (latest by date)](https://img.shields.io/github/v/tag/willcipriano/connery?style=for-the-badge)
![GitHub code size in bytes](https://img.shields.io/github/languages/code-size/willcipriano/Connery?style=for-the-badge)
![Website](https://img.shields.io/website?style=for-the-badge&url=http%3A%2F%2Fconnerylang.org)
![Travis (.org)](https://img.shields.io/travis/willcipriano/Connery?style=for-the-badge)