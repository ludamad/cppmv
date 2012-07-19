# Accepts <src> <dst> where either may be folders

src=$1
dst=$2
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [ $# -lt 2 ]; then
	echo "Expected usage: cppmv <src> <dst>"
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

	cp -r $src $dst #hack to make sure necessary folders exist
	
	for file in $movingfiles ; do
		updateargs+="$src/${file:2} $dst/${file:2} " # strip ./'s from start
	done

	for file in $movingfiles ; do
		if ! $DIR/cppmv-update $src/${file:2} $dst/${file:2} $updateargs ; then
			echo "Error in movement of $sf to $df - source files remain unchanged."; exit -1
		fi
	done

	for file in $movingfiles ; do rm $src/${file:2} ; done
	rm -r $src
else #Hack it to make it look like a dir with just our $src file in it
	updateargs="$src $dst"
	if [ -e "$dst" ]; then
		echo "$dst already exists, aborting."	
		exit -1
	elif ! $DIR/cppmv-update $src $dst $updateargs ; then
		echo "Error in movement of $sf to $df - source files remain unchanged."	
		exit -1
	fi
	rm $src
fi

for file in $allfiles ; do
	if [ -e $file ] ; then
		$DIR/cppmv-update $file $file $updateargs
	fi
done

