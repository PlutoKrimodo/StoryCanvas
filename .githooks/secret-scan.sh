#!/bin/sh
# 密钥形态扫描：被 pre-commit / pre-push 共用。
#
# 依赖：git + GNU grep（区间量词）+ awk + sed，不引入任何语言运行时、不下载二进制。
# 原则：只看新增行；命中只输出「文件:行号 + 变量名」，值一律不回显。
#
# 注意 mawk 不支持 {n,} 量词，因此模式匹配全部交给 grep -E，awk 只负责行号与脱敏。

# ==================== 关卡 B：内容模式 ====================
RE_SK='sk-[A-Za-z0-9]{20,}'
RE_KV="[A-Za-z_][A-Za-z0-9_]*(API_KEY|API_SECRET|SECRET_KEY|SECRET|ACCESS_KEY|ACCESS_TOKEN|AUTH_TOKEN|[A-Z0-9_]*_TOKEN|PASSWORD|PASSWD|PRIVATE_KEY)[A-Za-z0-9_]*[[:space:]]*[=:][[:space:]]*[\"']?[A-Za-z0-9_.:+/=_-]{16,}"
RE_AUTH="Authorization[[:space:]]*:[[:space:]]*(Bearer|Basic)[[:space:]]+[A-Za-z0-9_.+/=-]{16,}"
RE_VITE="VITE_[A-Z0-9_]*(KEY|SECRET|TOKEN|PASSWORD|PASSWD)[A-Z0-9_]*[[:space:]]*[=:][[:space:]]*[\"']?[^[:space:]\"']{6,}"
RE_CONTENT="$RE_SK|$RE_KV|$RE_AUTH|$RE_VITE"

# 行尾 `# allow-secret` 单行豁免
RE_ALLOW='allow-secret'

# ==================== 关卡 C：占位值白名单 ====================
RE_PLACEHOLDER='change_?me|changeme|your[_-][a-z0-9_]*|[_-]here$|placeholder|sample|fake|dummy|test[_-]?key|demo[_-]?key|example|xxxxxxxx|<[^>]{2,}>|\$\{?[A-Za-z0-9_]+'

# ==================== 关卡 A：文件名 ====================
RE_BAD_NAME='(^|/)\.env(\.[A-Za-z0-9-]+)?$|\.(pem|key|p12|jks|keystore|kdbx)$|id_[rs]a([0-9]+)?$|credentials|\.npmrc$|\.pypirc$|settings\.local\.json$|secrets\.ya?ml$'
RE_GOOD_NAME='(\.env|\.template|\.prose|\.model)\.(example|sample|template|dist)$'

# 从统一 diff 里取「新增行」，输出  文件<TAB>新行号<TAB>内容
scan_added_lines_from_diff() {
    awk '
        /^--- /  { next }
        /^\+\+\+ b\// { file = substr($0, 7); sub(/[ \t].*$/, "", file); next }
        /^@@ / {
            if (match($0, /\+[0-9]+/)) ln = substr($0, RSTART + 1, RLENGTH - 1) - 1
            next
        }
        /^\\ / { next }
        /^\+ / { next }
        /^[+]/ { ln++; printf "%s\t%d\t%s\n", file, ln, substr($0, 2); next }
        /^[-]/ { next }
         { ln++ }
    '
}

# 把命中行脱敏：值换成 ***，只留变量名
mask_hits() {
    sed -E -e "s#(sk-)[A-Za-z0-9]{20,}#\\1***#g" \
           -e 's/[[:space:]]*[=:][[:space:]]*["'\'']?.*$/ = ***（值已隐藏）/' \
           -e 's/\t/:/g'
}

# stdin：候选记录（文件<TAB>行号<TAB>内容）；stdout：脱敏后的命中
#
# 关键：模式匹配与白名单只看「内容」字段，路径/行号不参与判断。
# 早先整行参与匹配时，ai-service/.env.example 这类路径里的 "example" 会命中占位白名单，
# 把写进模板文件的真实 key 一并放行（实测漏检）。
filter_secret_lines() {
    _loc=$(mktemp) _con=$(mktemp) _sel=$(mktemp) || return 0
    awk -v LOC="$_loc" -v CON="$_con" '
        {
            i = index($0, "\t")
            j = index(substr($0, i + 1), "\t")
            if (i == 0 || j == 0) next
            base = i + j
            print substr($0, 1, base - 1) > LOC
            print substr($0, base + 1)    > CON
        }
    '
    grep -nE "$RE_CONTENT" "$_con" 2>/dev/null \
        | grep -vE "$RE_ALLOW|$RE_PLACEHOLDER" \
        | cut -d: -f1 > "$_sel" 2>/dev/null
    if [ -s "$_sel" ]; then
        awk -v IDX="$_sel" '
            BEGIN { while ((getline i < IDX) > 0) need[i] = 1; close(IDX) }
            FNR == NR { if (need[FNR]) content[FNR] = $0; next }
            need[FNR] { print $0 "\t" content[FNR] }
        ' "$_con" "$_loc" | mask_hits
    fi
    rm -f "$_loc" "$_con" "$_sel"
    return 0
}

# 待检查的文件名（每行一个）→ 命中即打印
check_names() {
    bad=$(printf '%s\n' "$@" \
          | grep -Ei "$RE_BAD_NAME" \
          | grep -Eiv "$RE_GOOD_NAME")
    if [ -n "$bad" ]; then
        printf '%s\n' "$bad" | sed 's/^/  ✗ 敏感文件名：/'
        return 1
    fi
    return 0
}
