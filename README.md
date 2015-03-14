networking
=========
A sync/async networking library

## Build
You have to download one other repo to build netwoking library
  * [common](https://github.com/allenbo/common.git)

```
# make a workspace directory
cd ~ && mkdir workspace && cd workspace
# clone common and networking reps
git clone git@github.com:allenbo/common.git
git clone git@github.com:allenbo/networking.git
# compile networking
cd networking && make
```

A networking library called libnetworking.a will be generated under the directory. When you are using netwoking in other projects, be sure to include `common` too.
