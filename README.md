kospam is an open source email content filter application

## Features

- statistical (inverse chi-square) antispam module
- email black-, and whitelists
- zombie detection
- trap email addresses (minefield)

kospam can be used as an after queue content filter with Postfix
(including as an appliance) and Exim or via the maildrop local
delivery agent (LDA).


## Installation

Create a mysql database

```
create database kospam character set 'utf8';
grant all privileges on kospam.* to kospam identified by 'kospam123';
flush privileges;
```

## History

Kospam is a reboot of the previous clapf project
