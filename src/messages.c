/* ========================================================================

   $File: messages.c
   $Date: 2026/11/23
   $Revision: 1

   Error information is a side-channel, not a special return path. The log is
   a linked list of messages pushed from one arena. A zero-initialized
   message_list is an empty list, so every pass can append unconditionally and
   the platform layer can print the log at one shallow boundary.

   ======================================================================== */

#include "defs.h"

void
InitMessageLog(message_log *Log, memory_arena *Arena)
{
    ZeroStruct(*Log);
    Log->Arena = Arena;
}

void
PushMessageV(message_log *Log, message_kind Kind, source_file *Source,
             u32 Offset, char *Origin, char *Format, va_list Args)
{
    if(!Log || !Log->Arena)
    {
        return;
    }

    va_list LengthArgs;
    va_copy(LengthArgs, Args);
    int Formatted = vsnprintf(0, 0, Format, LengthArgs);
    va_end(LengthArgs);

    if(Formatted < 0)
    {
        return;
    }

    memory_index TextSize = (memory_index)Formatted + 1;
    memory_index Need = sizeof(message) + TextSize + 64;
    if((Log->Arena->Used + Need) > Log->Arena->Size)
    {
        /* The log is a bounded side-channel. Dropping a diagnostic changes no
         * compiler result, so it must not become a new failure codepath. */
        ++Log->Dropped;
        return;
    }

    char *Text = PushArray(Log->Arena, TextSize, char);
    vsnprintf(Text, TextSize, Format, Args);

    message *Msg = PushStruct(Log->Arena, message);
    ZeroStruct(*Msg);
    Msg->Kind = Kind;
    Msg->Text = MakeString(Text, (u32)Formatted);
    Msg->Source = Source ? Source : &NilSource;
    Msg->Offset = Offset;
    Msg->Origin = Origin ? Origin : "compiler";

    if(Log->List.Last)
    {
        Log->List.Last->Next = Msg;
    }
    else
    {
        Log->List.First = Msg;
    }
    Log->List.Last = Msg;
    ++Log->List.Count;
    if(Kind == Msg_Error)
    {
        ++Log->List.ErrorCount;
    }
}

void
PushMessage(message_log *Log, message_kind Kind, source_file *Source,
            u32 Offset, char *Origin, char *Format, ...)
{
    va_list Args;
    va_start(Args, Format);
    PushMessageV(Log, Kind, Source, Offset, Origin, Format, Args);
    va_end(Args);
}

internal void
SourcePosition(source_file *Source, u32 Offset, u32 *Line, u32 *Column)
{
    u32 ResultLine = 1;
    u32 ResultColumn = 1;
    char *Data = Source && Source->Data ? Source->Data : "";
    u32 Size = Source ? Source->Size : 0;
    u32 Limit = Minimum(Offset, Size);

    for(u32 Index = 0; Index < Limit; ++Index)
    {
        if(Data[Index] == '\n')
        {
            ++ResultLine;
            ResultColumn = 1;
        }
        else
        {
            ++ResultColumn;
        }
    }

    *Line = ResultLine;
    *Column = ResultColumn;
}

void
PrintMessages(message_log *Log, FILE *Stream)
{
    if(!Log || !Stream)
    {
        return;
    }

    for(message *Msg = Log->List.First; Msg; Msg = Msg->Next)
    {
        char *Prefix = "note";
        if(Msg->Kind == Msg_Warning)
        {
            Prefix = "warning";
        }
        else if(Msg->Kind == Msg_Error)
        {
            Prefix = "error";
        }

        source_file *Source = Msg->Source ? Msg->Source : &NilSource;
        if(Msg->Offset != SourceOffsetNone)
        {
            u32 Line = 0;
            u32 Column = 0;
            SourcePosition(Source, Msg->Offset, &Line, &Column);
            fprintf(Stream, "%s:%u:%u: %s: %s: ", Source->FileName, Line,
                    Column, Prefix, Msg->Origin);
        }
        else
        {
            fprintf(Stream, "%s: %s: %s: ", Source->FileName, Prefix,
                    Msg->Origin);
        }

        if(Msg->Text.Data && Msg->Text.Count)
        {
            fprintf(Stream, "%.*s\n", (int)Msg->Text.Count, Msg->Text.Data);
        }
        else
        {
            fprintf(Stream, "\n");
        }
    }

    if(Log->Dropped)
    {
        fprintf(Stream, "%s: warning: compiler: %u diagnostic messages dropped\n",
                NilSource.FileName, Log->Dropped);
    }
}

b32
MessageLogHasErrors(message_log *Log)
{
    b32 Result = Log && (Log->List.ErrorCount != 0);
    return(Result);
}
