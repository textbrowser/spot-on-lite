#!/usr/bin/env sh

# Alexis Megas.

# If git has locked the local directory, a git command will fail.
# We will not remove the lock.

# If the local directory already exists, a git-clone will fail.
# We will not remove the local directory!

# We will not correct GIT errors.

if [ -z "$(which git)" ]
then
    echo "Please install git. Bye!"
    exit 1
fi

if [ -z ${GIT_A} ]
then
    echo "Please export GIT_A. Bye!"
    exit 1
fi

if [ -z ${GIT_LOCAL_DIRECTORY} ]
then
    echo "Please export GIT_LOCAL_DIRECTORY. Bye!"
    exit 1
fi

if [ -z ${GIT_SITE_CLONE} ]
then
    echo "Please export GIT_SITE_CLONE. Bye!"
    exit 1
fi

if [ -z ${GIT_SITE_PUSH} ]
then
    echo "Please export GIT_SITE_PUSH. Bye!"
    exit 1
fi

if [ -z ${GIT_T} ]
then
    echo "Please export GIT_T. Bye!"
    exit 1
fi

local_directory="${GIT_LOCAL_DIRECTORY}"
site_clone=$(eval "echo ${GIT_SITE_CLONE}")

if [ ! -e "$local_directory" ]
then
    echo "Cloning $site_clone into $local_directory."
    git clone --depth 1 -q "$site_clone" "$local_directory" \
	1>/dev/null 2>/dev/null

    rc=$?

    if [ ! $rc -eq 0 ]
    then
	echo "GIT-CLONE failed! Bye!"
	exit $rc
    else
	echo "GIT-CLONE finished! Great!"
    fi
fi

echo "Setting $local_directory as the current directory."
cd "$local_directory"

if [ ! $? -eq 0 ]
then
    echo "CD failed! Cannot set current directory! Bye!"
    exit 1
fi

if [ ! -e "$local_directory/.git" ]
then
    echo "The directory $local_directory/.git does not exist!"
    echo "Cloning $site_clone into $local_directory/tmp.d."
    rm -fr tmp.d 2>/dev/null
    git clone --depth 1 -q "$site_clone" tmp.d 1>/dev/null 2>/dev/null

    rc=$?

    if [ ! $rc -eq 0 ]
    then
	echo "GIT-CLONE failed! Bye!"
	exit $rc
    else
	echo "GIT-CLONE finished! Great!"
    fi

    echo "Moving tmp.d/.git into the current directory."
    mv tmp.d/.git . 2>/dev/null

    rc=$?

    if [ ! $rc -eq 0 ]
    then
	echo "MV failed! Bye!"
	exit $rc
    else
	echo "MV finished! Great!"
    fi

    rm -fr tmp.d
    echo "Checking out remote repository."
    git checkout . 1>/dev/null 2>/dev/null

    rc=$?

    if [ ! $rc -eq 0 ]
    then
	echo "GIT-CHECKOUT failed! Bye!"
	exit $rc
    else
	echo "GIT-CHECKOUT finished! Great!"
    fi
fi

# Remove Smoke files older than one minute.

echo "Removing Smoke files older than one minute."
find "$local_directory" \
     ! -path "*.git*" \
     -mmin +1 \
     -name "*Smoke*.txt" \
     -type f \
     -exec rm -f {} 2>/dev/null \;

# Remove files older than fifteen days.

echo "Removing files older than fifteen days."
find "$local_directory" \
     ! -path "*.git*" \
     -mtime +15 \
     -name "PrisonBlues*.*" \
     -type f \
     -exec rm -f {} 2>/dev/null \;

# Merge.

echo "Instructing GIT to avoid the rebase strategy. Merge changes instead."
git config pull.rebase false 1>/dev/null 2>/dev/null

if [ ! $? -eq 0 ]
then
    echo "GIT-CONFIG failed! Continuing!"
else
    echo "GIT-CONFIG finished! Great!"
fi

# Pull?

echo "Issuing a GIT-PULL request."
git pull --no-log 1>/dev/null 2>/dev/null

rc=$?

if [ $rc -eq 0 ]
then
    echo "GIT-PULL finished! Great!"
    echo "Determining if there are local revisions."
    rc=$(git ls-files --deleted --exclude-standard --others \
	     2>/dev/null | wc -l)
    site_push=$(eval "echo ${GIT_SITE_PUSH}")

    if [ $rc -lt 1 ]
    then
	echo "We do not have local revisions! Bye!"

	if [ ! -z "$(git status | grep 'git push' 2>/dev/null)" ]
	then
	    echo "A git-push is required!"
	    git push "$site_push" 1>/dev/null 2>/dev/null
	fi

	echo "Cleaning the local directory."
	git clean -df . 1>/dev/null 2>/dev/null
	exit 0
    fi

    echo "Adding local GPG files."
    git add --all */*.gpg 1>/dev/null 2>/dev/null

    rc=$?

    if [ ! $rc -eq 0 ]
    then
	echo "GIT-ADD-GPG failed! Continuing!"
    fi

    echo "Adding local text files."
    git add --all */*.txt 1>/dev/null 2>/dev/null

    rc=$?

    if [ ! $rc -eq 0 ]
    then
	echo "GIT-ADD-TXT failed! Continuing!"
    fi

    echo "Committing new data."
    git commit -a -m "New data." 1>/dev/null 2>/dev/null

    rc=$?

    if [ ! $rc -eq 0 ]
    then
	echo "GIT-COMMIT failed! Bye!"
	exit $rc
    else
	echo "GIT-COMMIT finished! Great!"
    fi

    echo "Issuing a GIT-PULL request."
    git pull --no-log 1>/dev/null 2>/dev/null

    if [ ! $? -eq 0 ]
    then
	echo "GIT-PULL failed! Continuing!"
    else
	echo "GIT-PULL finished! Great!"
    fi

    echo "Issuing a GIT-PUSH request."
    git push "$site_push" 1>/dev/null 2>/dev/null

    rc=$?

    if [ ! $rc -eq 0 ]
    then
	echo "GIT-PUSH failed! Bye!"
	exit $rc
    else
	echo "GIT-PUSH finished! Great!"
    fi
else
    echo "GIT-PULL failed! Bye!"
    exit $rc
fi

echo "Optimizing the local GIT directory."
git gc 1>/dev/null 2>/dev/null
echo "$0 completed successfully!"
exit 0
