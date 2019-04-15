module Demo
{
    interface ServerService
    {
        void requireService(string ident, string s);
    }

    interface ClientCallback
    {
        void callback(string s);
    }
}
