# Bowsprit

[![Build Status](https://img.shields.io/travis/redjack/bowsprit/develop.svg)](https://travis-ci.org/redjack/bowsprit)

Bowsprit is a library for generating [collectd](https://collectd.org/)-style
statistics from a C application.  Normally, you'll have a collectd instance
running somewhere that you can send the statistics too (either on the same
machine or nearby on the local network).  The library also provides some basic
support for collecting and displaying the statistics without a collectd
instance, which can be useful during development and in your test cases.

Bowsprit follows the collectd naming conventions as much as possible.  You
create one or more *measurements*, each of which belongs to a *plugin*.

Each measurement has a *type name* and one or more *values*.  Measurements can
also have an optional *type instance*, which can be used to distinguish between
multiple measurements in the same plugin that all have the same type name.

The measurement's type name is an index into collectd's
[types.db](https://collectd.org/documentation/manpages/types.db.5.shtml) file,
and defines how many values the measurement has, and the *kind* of each value.
There are [four available
kinds](https://collectd.org/wiki/index.php/Data_source): *absolute*, *counter*,
*derive*, and *gauge*.  Gauge and derive are the most common.  Use a gauge for a
measurement that can go up and down arbitrarily, such as "current memory usage".
Use a derive for a counter that is not likely to overflow, such as "number of
packets processed".  A counter is similar to a derive, but is used for small
counters that are likely to wrap, such as a TCP sequence number; collectd will
take this into account when calculating rates.  An absolute is like a gauge that
resets to 0 every time it's read.

A plugin is a collection of measurements that all describe the same portion of
code.  (The name doesn't really have anything to do with loadable code modules;
it's just the collectd synonym for "component" or "subsystem".)  Plugins have a
*name* and an optional *instance*; just like with measurements, the instance can
be used to distinguish between multiple plugins with the same name.

As an application or library designer, it's your responsibility to choose plugin
and measurement names that are descriptive and meaningful.  Your monitoring
processes will use these names as-is, so choose them with an eye towards making
your downstream logging and analytics as easy to understand as possible.
