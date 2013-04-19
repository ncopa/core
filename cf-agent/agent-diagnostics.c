#include "agent-diagnostics.h"

#include "alloc.h"
#include "crypto.h"
#include "files_interfaces.h"
#include "string_lib.h"
#include "bootstrap.h"

#include <assert.h>

AgentDiagnosticsResult AgentDiagnosticsResultNew(bool success, char *message)
{
    return (AgentDiagnosticsResult) { success, message };
}

static void AgentDiagnosticsResultDestroy(AgentDiagnosticsResult result)
{
    free(result.message);
}

void AgentDiagnosticsRun(const char *workdir, const AgentDiagnosticCheck checks[], Writer *output)
{
    {
        char diagnostics_path[CF_BUFSIZE] = { 0 };
        snprintf(diagnostics_path, CF_BUFSIZE, "%s/diagnostics", workdir);
        MapName(diagnostics_path);

        struct stat sb;
        if (cfstat(diagnostics_path, &sb) != 0)
        {
            if (mkdir(diagnostics_path, DEFAULTMODE) != 0)
            {
                WriterWriteF(output, "Cannot create diagnostics output directory '%s'", diagnostics_path);
                return;
            }
        }
    }


    for (int i = 0; checks[i].description; i++)
    {
        AgentDiagnosticsResult result = checks[i].check(workdir);
        WriterWriteF(output, "[ %s ] %s: %s\n",
                     result.success ? "YES" : "NO ",
                     checks[i].description,
                     result.message);
        AgentDiagnosticsResultDestroy(result);
    }
}

AgentDiagnosticsResult AgentDiagnosticsCheckIsBootstrapped(const char *workdir)
{
    char *policy_server = GetPolicyServer(workdir);
    return AgentDiagnosticsResultNew(policy_server != NULL,
                                     policy_server != NULL ? policy_server : xstrdup("Not bootstrapped"));
}

AgentDiagnosticsResult AgentDiagnosticsCheckAmPolicyServer(const char *workdir)
{
    bool am_policy_server = GetAmPolicyServer(workdir);
    return AgentDiagnosticsResultNew(am_policy_server,
                                     am_policy_server ? xstrdup("Acting as a policy server") : xstrdup("Not acting as a policy server"));
}

AgentDiagnosticsResult AgentDiagnosticsCheckHavePrivateKey(const char *workdir)
{
    const char *path = PrivateKeyFile(workdir);
    assert(path);
    struct stat sb;
    return AgentDiagnosticsResultNew((cfstat(path, &sb) == 0), xstrdup(path));
}

AgentDiagnosticsResult AgentDiagnosticsCheckHavePublicKey(const char *workdir)
{
    const char *path = PublicKeyFile(workdir);
    assert(path);
    struct stat sb;
    return AgentDiagnosticsResultNew((cfstat(path, &sb) == 0), xstrdup(path));
}

const AgentDiagnosticCheck *AgentDiagnosticsAllChecks(void)
{
    static const AgentDiagnosticCheck checks[] =
    {
        { "Check that agent is bootstrapped", &AgentDiagnosticsCheckIsBootstrapped },
        { "Check if agent is acting as a policy server", &AgentDiagnosticsCheckAmPolicyServer },
        { "Check that private key exists", &AgentDiagnosticsCheckHavePrivateKey },
        { "Check that public key exists", &AgentDiagnosticsCheckHavePublicKey },

        { NULL, NULL }
    };

    return checks;
}