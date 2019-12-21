function hlightduer() {
cat <<EOF
use ". build/envsetup.sh" to add lightduerconfig command to your environment:
- lightduerconfig:
        lightduerconfig <vendor_name>_<product_name>-<build_variant>
        lightduerconfig <platform_name>_<compile_env_name>-<build_variant>

EOF
}

PLATFORM_CHOICES=(freertos linux)

# check to see if the supplied platform is valid
function check_platform()
{
    for v in ${PLATFORM_CHOICES[@]}
    do
        if [ "$v" = "$1" ]
        then
            return 0
        fi
    done
    return 1
}

COMPILE_ENV_CHOICES=(freertos linux)

# check to see if the supplied compile env is valid
function check_compile_env()
{
    for v in ${COMPILE_ENV_CHOICES[@]}
    do
        if [ "$v" = "$1" ]
        then
            return 0
        fi
    done
    return 1
}

function gettop
{
    local TOPFILE=build/envsetup.sh
    if [ -n "$TOP" -a -f "$TOP/$TOPFILE" ] ; then
        # The following circumlocution ensures we remove symlinks from TOP.
        (cd $TOP; PWD= /bin/pwd)
    else
        if [ -f $TOPFILE ] ; then
            # The following circumlocution (repeated below as well) ensures
            # that we record the true directory name and not one that is
            # faked up with symlink names.
            PWD= /bin/pwd
        else
            local HERE=$PWD
            T=
            while [ \( ! \( -f $TOPFILE \) \) -a \( $PWD != "/" \) ]; do
                \cd ..
                T=`PWD= /bin/pwd -P`
            done
            \cd $HERE
            if [ -f "$T/$TOPFILE" ]; then
                echo $T
            fi
        fi
    fi
}

# Clear this variable.
# It will be built up again when the configsetup.sh
# files are included at the end of this file.
unset CONFIG_MENU_CHOICES
function add_config_combo()
{
    local new_combo=$1
    local c
    for c in ${CONFIG_MENU_CHOICES[@]} ; do
        if [ "$new_combo" = "$c" ] ; then
            return
        fi
    done
    CONFIG_MENU_CHOICES=(${CONFIG_MENU_CHOICES[@]} $new_combo)
}

function print_config_menu()
{
    local uname=$(uname)
    echo
    echo "You're building on" $uname
    echo
    echo "config menu... pick one:"

    local i=1
    local choice
    for choice in ${CONFIG_MENU_CHOICES[@]}
    do
        echo "     $i. $choice"
        i=$(($i+1))
    done

    echo
}

function lightduerconfig()
{
    local answer

    if [ "$1" ] ; then
        answer=$1
    else
        print_config_menu
        echo -n "Which would you like? [1] "
        read answer
    fi

    local selection=

    if [ -z "$answer" ]
    then
        answer=1
    fi
    if (echo -n $answer | grep -q -e "^[0-9][0-9]*$")
    then
        if [ $answer -le ${#CONFIG_MENU_CHOICES[@]} ]
        then
            selection=${CONFIG_MENU_CHOICES[$(($answer-1))]}
        fi
    elif (echo -n $answer | grep -q -e "^[^\-][^\-]*-[^\-][^\-]*$")
    then
        selection=$answer
    fi

    if [ -z "$selection" ]
    then
        echo
        echo "Invalid config: $answer"
        return 1
    fi

    local variant=$(echo -n $selection | sed -e "s/^[^\-]*-//")
# PE: product or compile environment
# VP: vendor or platform
    local VP_PE=$(echo -n $selection | sed -e "s/-.*$//")
    local VP=$(echo -n $VP_PE| sed -e "s/_.*$//")
    local PE=$(echo -n $VP_PE| sed -e "s/.*_//")
    local T=$(gettop)
    local config_file=$T/build/device/$VP/$PE/$variant.mk
    if [ ! -f $config_file ]
    then
        echo
        echo "** Don't have a config for: '$VP_PE'"
        echo "** Do you have set the config $config_file?"
        return 1
    fi

    echo "******************************"
    echo TARGET_PLATFORM_OR_VENDOR=$VP
    echo TARGET_COMPILE_ENV_OR_PRODUCT=$PE
    echo TARGET_BUILD_VARIANT=$variant
    echo "******************************"

    export TARGET_PLATFORM_OR_VENDOR=$VP
    export TARGET_COMPILE_ENV_OR_PRODUCT=$PE
    export TARGET_BUILD_VARIANT=$variant
}

# Tab completion for config.
function _config()
{
    local cur prev opts
    COMPREPLY=()
    cur="${COMP_WORDS[COMP_CWORD]}"
    prev="${COMP_WORDS[COMP_CWORD-1]}"

    COMPREPLY=( $(compgen -W "${CONFIG_MENU_CHOICES[*]}" -- ${cur}) )
    return 0
}
complete -F _config config

function get_make_command()
{
  echo command make
}

function make()
{
    local start_time=$(date +"%s")
    $(get_make_command) "$@"
    local ret=$?
    local end_time=$(date +"%s")
    local tdiff=$(($end_time-$start_time))
    local hours=$(($tdiff / 3600 ))
    local mins=$((($tdiff % 3600) / 60))
    local secs=$(($tdiff % 60))
    local ncolors=$(tput colors 2>/dev/null)
    if [ -n "$ncolors" ] && [ $ncolors -ge 8 ]; then
        color_failed=$'\E'"[0;31m"
        color_success=$'\E'"[0;32m"
        color_reset=$'\E'"[00m"
    else
        color_failed=""
        color_success=""
        color_reset=""
    fi
    echo
    if [ $ret -eq 0 ] ; then
        echo -n "${color_success}#### make completed successfully "
    else
        echo -n "${color_failed}#### make failed to build some targets "
    fi
    if [ $hours -gt 0 ] ; then
        printf "(%02g:%02g:%02g (hh:mm:ss))" $hours $mins $secs
    elif [ $mins -gt 0 ] ; then
        printf "(%02g:%02g (mm:ss))" $mins $secs
    elif [ $secs -gt 0 ] ; then
        printf "(%s seconds)" $secs
    fi
    echo " ####${color_reset}"
    echo
    return $ret
}

if [ "x$SHELL" != "x/bin/bash" ]; then
    case `ps -o command -p $$` in
        *bash*)
            ;;
        *)
            echo "WARNING: Only bash is supported, use of other shell would lead to erroneous results"
            ;;
    esac
fi

# Execute the contents of any configsetup.sh files we can find.
for f in `test -d build/device && find -L build/device -maxdepth 4 -name 'configsetup.sh' 2> /dev/null | sort`
do
    echo "including $f"
    . $f
done
unset f
