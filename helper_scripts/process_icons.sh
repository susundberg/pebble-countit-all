#!/bin/bash

ls -1 images_new/|xargs -iXXX convert  -threshold 1% -negate images_new/XXX images_new2/XXX

