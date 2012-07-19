# Accepts <src> <dst> where either may be folders


## Organize arguments
argv=("$@")
argc=$#
flags=""
gitmv=false
updatecmake=false
for (( i = 0 ; i < $argc - 2; i++ )); do
	arg=${argv[$i]}
	if [ $arg = --git ]; then 
		gitmv=true; flags+="--self-write-only ";
	elif [ $arg = --cmake ]; then updatecmake=true; else
		flags+="$arg "
	fi
done

src=${argv[ (($argc-2)) ]}
dst=${argv[ (($argc-1)) ]}
## Organize arguments

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
cppmvupdate="$DIR/cppmv-update"

if [ $# -lt 2 ]; then
	echo "Expected usage: cppmv [--sort --git --cmake] <src> <dst>"
	exit -1
fi

if [ ! -e "$src" ]; then
	echo "$src does not exist, aborting."
	exit -1
fi

updateargs=""
allfiles=$(find . -regex '.*\.\(c\|cpp\|h\)$' -print)

if [ -d $src ]; then

	pushd $src > /dev/null
	movingfiles=$(find . -regex '.*\.\(c\|cpp\|h\)$' -print)
	popd > /dev/null

	for file in $movingfiles ; do
		df=$dst/${file:2}
		if [ -e $df ]; then echo "$df already exists, aborting."; exit -1; fi
	done

	if ! $gitmv ; then
		if [ ! -e $dst ]; then mkdir -p $dst ; fi
		cp -r $src/* $dst #hack to make sure necessary folders exist
	fi	

	for file in $movingfiles ; do
		updateargs+="$src/${file:2} $dst/${file:2} " # strip ./'s from start
	done

	for file in $movingfiles ; do
		if ! $cppmvupdate $flags $src/${file:2} $dst/${file:2} $updateargs ; then
			if [! $gitmv ]; then echo "Error in movement of $sf to $df - source files remain unchanged."
			else echo "Error in processing of $sf to $df - source files may have been changed, git revert recommended."; fi
			exit -1
		fi
	done

	if $gitmv ; then
		git mv $src $dst 
	else
		for file in $movingfiles ; do rm $src/${file:2} ; done
		rm -r $src
	fi
else #Hack it to make it look like a dir with just our $src file in it
	updateargs="$src $dst"
	if [ -e "$dst" ]; then
		echo "$dst already exists, aborting."	
		exit -1
	elif ! $cppmvupdate $flags $src $dst $updateargs ; then
		if ! $gitmv ; then echo "Error in movement of $sf to $df - source files remain unchanged."; 
		else echo "Error in processing of $sf to $df - source files may have been changed, 'git reset HEAD' recommended."; fi
		exit -1
	fi
	if $gitmv ; then git mv $src $dst; else rm $src; fi
fi

for file in $allfiles ; do
	if [ -e $file ] ; then
		$cppmvupdate $flags $file $file $updateargs
	fi
done

if $updatecmake && [ -e CMakeLists.txt ] ; then $cppmvupdate --cmake CMakeLists.txt CMakeLists.txt $updateargs ; fi

